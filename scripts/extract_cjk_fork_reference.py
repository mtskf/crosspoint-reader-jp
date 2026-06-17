#!/usr/bin/env python3
"""Extract reference data from the fork's cjk_ui_font_20.h for parity testing.

Re-uses the parser from scripts/convert_cjk_font_to_epdfontdata.py so the test
is unaffected by include-format quirks (namespace, PROGMEM, per-glyph comments).
"""
import argparse
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from convert_cjk_font_to_epdfontdata import parse_int_array  # shared helper


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--input', required=True)
    ap.add_argument('--output', required=True)
    args = ap.parse_args()
    source = pathlib.Path(args.input).read_text(encoding='utf-8')
    codepoints = parse_int_array(source, 'CJK_UI_CODEPOINTS')
    widths = parse_int_array(source, 'CJK_UI_GLYPH_WIDTHS')
    glyphs = parse_int_array(source, 'CJK_UI_GLYPHS')
    # Sanity checks — same invariants as the converter, applied to the reference data.
    assert len(widths) == len(codepoints), f"widths/codepoints mismatch: {len(widths)} vs {len(codepoints)}"
    assert len(glyphs) == len(codepoints) * 60, f"glyph bytes {len(glyphs)} != {len(codepoints)*60}"
    assert codepoints == sorted(codepoints), "Codepoints not sorted ascending"
    assert len(set(codepoints)) == len(codepoints), "Duplicate codepoints detected in fork header"
    assert all(1 <= w <= 20 for w in widths), \
        f"Width outside [1, 20]: {[w for w in widths if not (1 <= w <= 20)]}"
    out = [
        '#pragma once',
        '#include <cstdint>',
        f'inline constexpr int kForkRef_NumGlyphs = {len(codepoints)};',
        'inline constexpr uint16_t kForkRef_Codepoints[] = {' + ','.join(str(c) for c in codepoints) + '};',
        'inline constexpr uint8_t  kForkRef_Widths[]     = {' + ','.join(str(w) for w in widths)     + '};',
        'inline constexpr uint8_t  kForkRef_Glyphs[]     = {' + ','.join(str(b) for b in glyphs)     + '};',
        '// Alias for test (previously CJK_UI_FONT_GLYPH_COUNT_VAL)',
        f'inline constexpr uint16_t CJK_UI_FONT_GLYPH_COUNT_VAL = {len(codepoints)};',
    ]
    pathlib.Path(args.output).write_text('\n'.join(out) + '\n', encoding='utf-8')
    print(f"Extracted {len(codepoints)} codepoints, {len(widths)} widths, {len(glyphs)} glyph bytes")


if __name__ == '__main__':
    main()
