#!/usr/bin/env python3
"""Manual sanity check: compare cjk_ui_20.cpp bitmap decode vs fork source.

Usage: python3 scripts/check_cjk_font_data.py
Requires: /tmp/crosspoint-jp present, lib/EpdFont/builtinFonts/cjk_ui_20.cpp generated.

For a sample of codepoints, decode the EpdFontData bitmap and the fork bitmap and compare.
Prints PASS/FAIL for each codepoint checked.
"""
import re
import sys
from pathlib import Path

FONT_HEIGHT = 20
BYTES_PER_ROW = 3


def strip_comments(source):
    """Remove C/C++ comments (the fork header annotates each glyph with a
    `// U+XXXX (c)` comment; the literal `}` in the U+007D comment would break
    the brace-body capture below)."""
    source = re.sub(r'/\*.*?\*/', '', source, flags=re.S)
    source = re.sub(r'//[^\n]*', '', source)
    return source


def parse_int_array(source, name):
    """Parse a C-style array definition for `name`, accepting both
    0x-prefixed hex literals AND decimal literals (used by CJK_UI_GLYPH_WIDTHS).
    The bracket may be empty `[]` (fork source) or sized `[169240]` (generated cpp)."""
    source = strip_comments(source)
    m = re.search(rf'{re.escape(name)}\s*\[\s*\d*\s*\]\s*(?:PROGMEM\s*)?=\s*\{{([^}}]*)\}}', source)
    if not m:
        raise ValueError(f"Array '{name}' not found")
    body = m.group(1)
    tokens = re.findall(r'(?:0[xX][0-9A-Fa-f]+|\d+)[uU]?', body)
    return [int(t.rstrip('uU'), 0) for t in tokens]


def fork_bits(glyph_data, idx, w):
    bits = []
    off = idx * 60
    for y in range(FONT_HEIGHT):
        for x in range(w):
            b = glyph_data[off + y * BYTES_PER_ROW + (x >> 3)]
            bits.append((b >> (7 - (x & 7))) & 1)
    return bits


def epd_bits(bitmap, data_offset, data_length, w):
    bits = []
    for y in range(FONT_HEIGHT):
        for x in range(w):
            pos = y * w + x
            byte_idx = data_offset + (pos >> 3)
            if byte_idx >= data_offset + data_length:
                bits.append(0)
                continue
            b = bitmap[byte_idx]
            bits.append((b >> (7 - (pos & 7))) & 1)
    return bits


def main():
    fork_src = Path('/tmp/crosspoint-jp/lib/GfxRenderer/cjk_ui_font_20.h').read_text(encoding='utf-8')
    epd_src = Path('lib/EpdFont/builtinFonts/cjk_ui_20.cpp').read_text(encoding='utf-8')

    codepoints = parse_int_array(fork_src, 'CJK_UI_CODEPOINTS')
    widths = parse_int_array(fork_src, 'CJK_UI_GLYPH_WIDTHS')
    fork_glyphs = parse_int_array(fork_src, 'CJK_UI_GLYPHS')
    epd_bitmap = parse_int_array(epd_src, 'cjk_ui_20_bitmap')

    # Parse glyph metadata from .cpp
    glyph_entries = re.findall(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\}', epd_src)
    if len(glyph_entries) < len(codepoints):
        print(f"ERROR: found {len(glyph_entries)} glyph entries, expected {len(codepoints)}")
        sys.exit(1)

    # Sample: first 10, last 10, and a few from the CJK block
    sample_indices = list(range(10)) + list(range(len(codepoints)-10, len(codepoints)))
    # Add hiragana あ, kanji 日 if present
    for cp in [0x3042, 0x65E5]:
        if cp in codepoints:
            sample_indices.append(codepoints.index(cp))

    failures = 0
    for i in sorted(set(sample_indices)):
        cp = codepoints[i]
        w = widths[i]
        entry = glyph_entries[i]
        g_w, g_h, adv, left, top, dl, do_ = [int(x) for x in entry]

        fork_b = fork_bits(fork_glyphs, i, w)
        epd_b = epd_bits(epd_bitmap, do_, dl, g_w)

        if fork_b == epd_b:
            print(f"PASS U+{cp:04X} (idx={i} w={w})")
        else:
            print(f"FAIL U+{cp:04X} (idx={i} w={w}): {sum(a!=b for a,b in zip(fork_b,epd_b))} bit differences")
            failures += 1

    if failures == 0:
        print(f"\nAll {len(set(sample_indices))} sampled codepoints match.")
    else:
        print(f"\n{failures} codepoints FAILED bit comparison.", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
