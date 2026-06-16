#pragma once
#include <Utf8.h>            // utf8IsCjkBreakable
#include "EpdFontData.h"     // EpdGlyph
#include "cjk_ui_font_20.h"  // CjkUiFont20::*

// Built-in 20px CJK UI font fallback for UI fonts. EpdFont::getGlyph returns a
// synthesized EpdGlyph from a small ring; GfxRenderer detects it via isSynthesized()
// and decodes the PROGMEM bitmap directly (the format differs from EpdFontData).
namespace CjkUiFallback {

// Baseline contract: the 20px box sits with this many px above the baseline.
// drawText passes cursorY = y + getFontAscenderSize(); renderCharImpl uses
// (cursorY - top). top = HEIGHT-4 places ~4px of the box below the baseline.
inline constexpr int16_t CJK_TOP = CjkUiFont20::CJK_UI_FONT_HEIGHT - 4;  // 16

inline constexpr int RING = 8;  // a few glyphs may be live at once (combining marks, measure+draw)
inline EpdGlyph g_ring[RING];
inline int g_ringPos = 0;

// True only for real CJK codepoints the CJK font covers. Never true for cp < 0x80,
// so ASCII/Latin stay on the primary UI font.
inline bool shouldUse(uint32_t cp) {
  return utf8IsCjkBreakable(cp) && CjkUiFont20::hasCjkUiGlyph(cp);
}

inline const EpdGlyph* makeGlyph(uint32_t cp) {
  // Use the proportional cell width for BOTH bbox width and advance so getTextBounds()
  // (which measures bbox = left + width, EpdFont.cpp:60) equals the draw advance exactly.
  // No fork-style ">=20 -> 18" tightening — that made measurement and drawing diverge.
  const uint8_t w = CjkUiFont20::getCjkUiGlyphWidth(cp);  // ~20 for full-width kanji
  EpdGlyph& g = g_ring[g_ringPos];
  g_ringPos = (g_ringPos + 1) % RING;
  g.width = w;                                 // bbox width == advance
  g.height = CjkUiFont20::CJK_UI_FONT_HEIGHT;  // 20
  g.advanceX = static_cast<uint16_t>(w) << 4;  // 12.4 fixed-point; advance == bbox width
  g.left = 0;
  g.top = CJK_TOP;
  g.dataLength = 0;       // unused for CJK glyphs
  g.dataOffset = cp;      // carry cp so the blitter can fetch the PROGMEM bitmap
  return &g;
}

inline bool isSynthesized(const EpdGlyph* g) {
  // Relational comparison of pointers into *different* array objects is UB, so compare
  // the converted integer addresses (well-defined integer comparison on a flat address space).
  if (g == nullptr) return false;
  const auto p = reinterpret_cast<uintptr_t>(g);
  return p >= reinterpret_cast<uintptr_t>(&g_ring[0]) && p < reinterpret_cast<uintptr_t>(&g_ring[RING]);
}

}  // namespace CjkUiFallback
