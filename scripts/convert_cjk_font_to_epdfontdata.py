#!/usr/bin/env python3
"""Convert cjk_ui_font_20.h (fork format) to EpdFontData .h/.cpp pair.

Source format: CJK_UI_CODEPOINTS[], CJK_UI_GLYPH_WIDTHS[], CJK_UI_GLYPHS[]
  - Each glyph: 20 rows x 3 bytes/row, MSB-first, pixel (x,y) at
    glyphs[idx*60 + y*3 + (x>>3)] bit 7-(x&7)
  - Proportional width in CJK_UI_GLYPH_WIDTHS[idx]

Target format: EpdFontData (lib/EpdFont/EpdFontData.h)
  - 1-bit tight packing, no row padding, MSB first
  - pixel (x,y) for glyph with width w: byte (y*w+x)>>3, bit 7-((y*w+x)&7)
  - advanceX = width << 4 (12.4 fixed-point)
  - top = 16, height = 20, left = 0
  - EpdFontData: advanceY=24, ascender=16, descender=4, is2Bit=false
"""
import argparse
import hashlib
import re
import sys
from pathlib import Path

FONT_HEIGHT = 20
BYTES_PER_ROW = 3  # source: 3 bytes per row (24 bits, only leftmost 20 used)
BYTES_PER_CHAR = 60  # FONT_HEIGHT * BYTES_PER_ROW
ADVANCE_Y = 24
ASCENDER = 16
DESCENDER = 4


def strip_comments(source: str) -> str:
    """Remove C/C++ comments. The fork header annotates every glyph in
    CJK_UI_GLYPHS with a `// U+XXXX (c)` comment; left in place these would
    (a) inject spurious tokens (e.g. `0021` from `U+0021`) and (b) the literal
    `}` in the comment for U+007D would prematurely terminate the brace-body
    capture in parse_int_array. Stripping first makes the parser robust."""
    source = re.sub(r'/\*.*?\*/', '', source, flags=re.S)
    source = re.sub(r'//[^\n]*', '', source)
    return source


def parse_int_array(source: str, name: str) -> list[int]:
    """Parse a C-style array definition for `name`, accepting both
    0x-prefixed hex literals AND decimal literals (used by CJK_UI_GLYPH_WIDTHS).
    Strips trailing 'u'/'U' suffixes if present. Comments are removed first."""
    source = strip_comments(source)
    # Match the array body: const ... NAME[] PROGMEM = { ... };
    m = re.search(rf'{re.escape(name)}\s*\[\s*\]\s*(?:PROGMEM\s*)?=\s*\{{([^}}]*)\}}', source)
    if not m:
        raise ValueError(f"Array '{name}' not found in source")
    body = m.group(1)
    # Tokenize on commas/whitespace, accept 0x... or decimal, drop trailing u/U.
    tokens = re.findall(r'(?:0[xX][0-9A-Fa-f]+|\d+)[uU]?', body)
    return [int(t.rstrip('uU'), 0) for t in tokens]  # int(s, 0) auto-detects 0x vs decimal


def repack_glyph(source_data: list[int], glyph_idx: int, width: int) -> bytes:
    """Repack one glyph from 3-byte-row MSB-first to tight 1-bit packing."""
    bits = []
    offset = glyph_idx * BYTES_PER_CHAR
    for y in range(FONT_HEIGHT):
        for x in range(width):
            byte_idx = offset + y * BYTES_PER_ROW + (x >> 3)
            bit = (source_data[byte_idx] >> (7 - (x & 7))) & 1
            bits.append(bit)
    # Pack bits MSB-first into bytes
    result = bytearray()
    for i in range(0, len(bits), 8):
        chunk = bits[i:i+8]
        while len(chunk) < 8:
            chunk.append(0)
        byte_val = 0
        for bit in chunk:
            byte_val = (byte_val << 1) | bit
        result.append(byte_val)
    return bytes(result)


def build_intervals(codepoints: list[int]) -> list[tuple[int, int, int]]:
    """Build EpdUnicodeInterval list from sorted codepoint array.

    Returns list of (first_cp, last_cp, glyph_array_offset).
    Contiguous codepoints are merged into one interval; gaps break intervals.
    """
    if not codepoints:
        return []
    intervals = []
    start = codepoints[0]
    prev = codepoints[0]
    interval_start_offset = 0
    for i, cp in enumerate(codepoints[1:], 1):
        if cp == prev + 1:
            prev = cp
        else:
            intervals.append((start, prev, interval_start_offset))
            start = cp
            prev = cp
            interval_start_offset = i
    intervals.append((start, prev, interval_start_offset))
    return intervals


def validate_arrays(codepoints: list[int], widths: list[int], glyph_data: list[int]) -> None:
    """Fail-fast validation shared by the converter and the parity-reference
    extractor. Uses explicit sys.exit (not assert) so the checks survive
    `python -O` / PYTHONOPTIMIZE, which strips assert statements."""
    if not codepoints or not widths or not glyph_data:
        print(f"ERROR: empty array parsed (codepoints={len(codepoints)}, "
              f"widths={len(widths)}, glyphs={len(glyph_data)}) — check fork header format",
              file=sys.stderr)
        sys.exit(1)
    if len(codepoints) != len(widths):
        print(f"ERROR: codepoint count {len(codepoints)} != width count {len(widths)}", file=sys.stderr)
        sys.exit(1)
    if len(glyph_data) != len(codepoints) * BYTES_PER_CHAR:
        print(f"ERROR: glyph byte count {len(glyph_data)} != {len(codepoints)} * {BYTES_PER_CHAR}", file=sys.stderr)
        sys.exit(1)
    if codepoints != sorted(codepoints):
        print("ERROR: codepoints must be sorted ascending", file=sys.stderr)
        sys.exit(1)
    if len(set(codepoints)) != len(codepoints):
        print("ERROR: duplicate codepoints detected", file=sys.stderr)
        sys.exit(1)
    bad = [w for w in widths if not 1 <= w <= 20]
    if bad:
        print(f"ERROR: width out of range [1,20]: {bad}", file=sys.stderr)
        sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, help='Path to cjk_ui_font_20.h')
    parser.add_argument('--output-dir', required=True, help='Output directory for cjk_ui_20.h and cjk_ui_20.cpp')
    args = parser.parse_args()

    source = Path(args.input).read_text(encoding='utf-8')
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # SHA-256 of the input lets CI detect drift between the committed artifact and
    # the fork source (the artifact is checked in; the fork source is not).
    source_hash = hashlib.sha256(Path(args.input).read_bytes()).hexdigest()
    header_comment = [
        '// Auto-generated by scripts/convert_cjk_font_to_epdfontdata.py',
        f'// Source: {args.input}',
        f'// sha256:{source_hash}',
        '// Do not edit manually — re-run the converter to regenerate.',
    ]

    codepoints = parse_int_array(source, 'CJK_UI_CODEPOINTS')
    widths = parse_int_array(source, 'CJK_UI_GLYPH_WIDTHS')
    glyph_data = parse_int_array(source, 'CJK_UI_GLYPHS')

    validate_arrays(codepoints, widths, glyph_data)
    print(f"Parsed {len(codepoints)} codepoints; widths range [{min(widths)}, {max(widths)}]; expected width 12 (narrow), 20 (full)")

    n = len(codepoints)
    print(f"Converting {n} glyphs...")

    # Repack all bitmaps and build glyph metadata
    all_bitmap = bytearray()
    glyphs = []  # list of (width, height, advanceX, left, top, dataLength, dataOffset)
    for i in range(n):
        w = widths[i]
        bitmap = repack_glyph(glyph_data, i, w)
        data_offset = len(all_bitmap)
        data_length = len(bitmap)
        all_bitmap.extend(bitmap)
        glyphs.append((w, FONT_HEIGHT, w << 4, 0, ASCENDER, data_length, data_offset))

    intervals = build_intervals(codepoints)
    print(f"Generated {len(intervals)} unicode intervals, {len(all_bitmap)} bitmap bytes")

    # --- Write cjk_ui_20.cpp ---
    cpp_lines = list(header_comment)
    cpp_lines += [
        '#include "cjk_ui_20.h"',
        '#include <pgmspace.h>',
        '',
        f'// {len(all_bitmap)} bytes of 1-bit tight-packed glyph bitmaps ({n} glyphs)',
        f'const uint8_t cjk_ui_20_bitmap[{len(all_bitmap)}] PROGMEM = {{',
    ]
    hex_vals = [f'0x{b:02X}' for b in all_bitmap]
    for i in range(0, len(hex_vals), 16):
        cpp_lines.append('  ' + ', '.join(hex_vals[i:i+16]) + ',')
    cpp_lines.append('};')
    cpp_lines.append('')

    cpp_lines.append(f'// {n} glyphs: width, height, advanceX(12.4fp), left, top, dataLength, dataOffset')
    cpp_lines.append(f'const EpdGlyph cjk_ui_20_glyphs[{n}] PROGMEM = {{')
    for w, h, adv, left, top, dl, do_ in glyphs:
        cpp_lines.append(f'  {{{w}, {h}, {adv}, {left}, {top}, {dl}, {do_}}},')
    cpp_lines.append('};')
    cpp_lines.append('')

    cpp_lines.append(f'// {len(intervals)} contiguous unicode intervals')
    cpp_lines.append(f'const EpdUnicodeInterval cjk_ui_20_intervals[{len(intervals)}] PROGMEM = {{')
    for first, last, off in intervals:
        cpp_lines.append(f'  {{0x{first:04X}, 0x{last:04X}, {off}}},')
    cpp_lines.append('};')
    cpp_lines.append('')

    cpp_lines += [
        'const EpdFontData cjk_ui_20_font_data = {',
        '  .bitmap = cjk_ui_20_bitmap,',
        '  .glyph = cjk_ui_20_glyphs,',
        '  .intervals = cjk_ui_20_intervals,',
        f'  .intervalCount = {len(intervals)},',
        f'  .advanceY = {ADVANCE_Y},',
        f'  .ascender = {ASCENDER},',
        f'  .descender = {DESCENDER},',
        '  .is2Bit = false,',
        '  .groups = nullptr,',
        '  .groupCount = 0,',
        '  .glyphToGroup = nullptr,',
        '  .kernLeftClasses = nullptr,',
        '  .kernRightClasses = nullptr,',
        '  .kernMatrix = nullptr,',
        '  .kernLeftEntryCount = 0,',
        '  .kernRightEntryCount = 0,',
        '  .kernLeftClassCount = 0,',
        '  .kernRightClassCount = 0,',
        '  .ligaturePairs = nullptr,',
        '  .ligaturePairCount = 0,',
        '  .glyphMissHandler = nullptr,',
        '  .glyphMissCtx = nullptr,',
        '};',
        '',
    ]
    (out_dir / 'cjk_ui_20.cpp').write_text('\n'.join(cpp_lines), encoding='utf-8')

    # --- Write cjk_ui_20.h ---
    h_lines = list(header_comment)
    h_lines += [
        '#pragma once',
        '#include "EpdFontData.h"',
        '',
        f'// 20px CJK UI font: {n} glyphs, advanceY={ADVANCE_Y}, ascender={ASCENDER}, descender={DESCENDER}',
        'extern const EpdFontData cjk_ui_20_font_data;',
        f'static constexpr uint16_t CJK_UI_20_GLYPH_COUNT = {n};',
        '',
    ]
    (out_dir / 'cjk_ui_20.h').write_text('\n'.join(h_lines), encoding='utf-8')

    print(f"Written: {out_dir}/cjk_ui_20.h, {out_dir}/cjk_ui_20.cpp")
    print(f"Glyph count: {n}, Bitmap bytes: {len(all_bitmap)}, Intervals: {len(intervals)}")


if __name__ == '__main__':
    main()
