#!/usr/bin/env python3
"""Extract reference data from the fork's cjk_ui_font_20.h for parity testing.

Re-uses the parser from scripts/convert_cjk_font_to_epdfontdata.py so the test
is unaffected by include-format quirks (namespace, PROGMEM, per-glyph comments).
"""
import argparse
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from convert_cjk_font_to_epdfontdata import parse_int_array, validate_arrays  # shared helpers


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--input', required=True)
    ap.add_argument('--output', required=True)
    args = ap.parse_args()
    source = pathlib.Path(args.input).read_text(encoding='utf-8')
    codepoints = parse_int_array(source, 'CJK_UI_CODEPOINTS')
    widths = parse_int_array(source, 'CJK_UI_GLYPH_WIDTHS')
    glyphs = parse_int_array(source, 'CJK_UI_GLYPHS')
    # Same fail-fast invariants as the converter (sys.exit-based, -O-safe).
    validate_arrays(codepoints, widths, glyphs)
    out = [
        '#pragma once',
        '#include <cstdint>',
        'inline constexpr uint16_t kForkRef_Codepoints[] = {' + ','.join(str(c) for c in codepoints) + '};',
        'inline constexpr uint8_t  kForkRef_Widths[]     = {' + ','.join(str(w) for w in widths)     + '};',
        'inline constexpr uint8_t  kForkRef_Glyphs[]     = {' + ','.join(str(b) for b in glyphs)     + '};',
        '// Glyph count under the name the parity test expects.',
        f'inline constexpr uint16_t CJK_UI_FONT_GLYPH_COUNT_VAL = {len(codepoints)};',
    ]
    pathlib.Path(args.output).write_text('\n'.join(out) + '\n', encoding='utf-8')
    print(f"Extracted {len(codepoints)} codepoints, {len(widths)} widths, {len(glyphs)} glyph bytes")


if __name__ == '__main__':
    main()
