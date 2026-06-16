#pragma once
#include <Utf8.h>  // utf8IsCjkBreakable

#include <algorithm>  // std::max

#include "EpdFontData.h"     // EpdGlyph
#include "cjk_ui_font_20.h"  // CjkUiFont20::*

// Built-in 20px CJK UI font fallback for UI fonts. EpdFont::getGlyph returns a
// synthesized EpdGlyph from a small ring; GfxRenderer detects it via isSynthesized()
// and decodes the PROGMEM bitmap directly (the format differs from EpdFontData).
namespace CjkUiFallback {

// Baseline contract: the 20px box sits with this many px above the baseline.
// drawText passes cursorY = y + getFontAscenderSize(); renderCharImpl uses
// (cursorY - top). top = HEIGHT-4 places ~4px of the box below the baseline.
inline constexpr int16_t CJK_TOP = CjkUiFont20::CJK_UI_FONT_HEIGHT - 4;            // 16
inline constexpr int16_t CJK_DESCENT = CjkUiFont20::CJK_UI_FONT_HEIGHT - CJK_TOP;  // 4
// Minimum line height needed to fit the 20px box plus its 4px descent below the baseline.
// GfxRenderer::getLineHeight() floors UI-font advanceY to this when CJK fallback is on,
// so the constant lives next to CJK_TOP to keep the two coupled values in one place.
inline constexpr int16_t CJK_MIN_LINE_HEIGHT = CjkUiFont20::CJK_UI_FONT_HEIGHT + CJK_DESCENT;  // 24

// RING capacity rationale: getTextBounds() and drawText() each call getGlyph() per
// codepoint and consume the returned pointer immediately — only integer copies of
// width/left/top/advanceX/height are retained. Multiple synthesized glyph pointers
// may be simultaneously live within a single measure + draw call sequence, so RING
// must exceed the worst-case nesting depth (today ~3); 8 is conservative headroom.
// The ring wraps silently when exceeded — prefer too large over too small.
//
// Combining marks never reach this path: CjkUiFallback::shouldUse() gates on
// utf8IsCjkBreakable(), which excludes every combining-mark block (0x0300-0x036F,
// 0x1DC0-0x1DFF, 0x20D0-0x20FF, 0xFE20-0xFE2F).
//
// Concurrency contract: g_ring and g_ringPos are shared, unsynchronized globals.
// Callers must serialize via the renderer's RenderLock — every existing
// getTextWidth()/drawText() call site already runs on the render task while
// holding RenderLock. A future cross-task caller (e.g. layout planning from the
// main task's loop()) would race here and silently corrupt glyph metrics.
inline constexpr int RING = 8;
inline EpdGlyph g_ring[RING];
inline int g_ringPos = 0;

// True only for real CJK codepoints the CJK font covers. Never true for cp < 0x80,
// so ASCII/Latin stay on the primary UI font.
inline bool shouldUse(uint32_t cp) { return utf8IsCjkBreakable(cp) && CjkUiFont20::hasCjkUiGlyph(cp); }

inline const EpdGlyph* makeGlyph(uint32_t cp) {
  // Use the proportional cell width for BOTH bbox width and advance so getTextBounds()
  // (which accumulates maxX = cursorX + glyph->left + glyph->width) equals the per-glyph
  // draw advance exactly. We set left = 0 below so advance == width keeps measure and draw
  // in lockstep. No fork-style ">=20 -> 18" tightening — that made measurement and drawing diverge.
  const uint8_t w = CjkUiFont20::getCjkUiGlyphWidth(cp);  // ~20 for full-width kanji
  EpdGlyph& g = g_ring[g_ringPos];
  g_ringPos = (g_ringPos + 1) % RING;
  g.width = w;                                 // bbox width == advance
  g.height = CjkUiFont20::CJK_UI_FONT_HEIGHT;  // 20
  g.advanceX = static_cast<uint16_t>(w) << 4;  // 12.4 fixed-point; advance == bbox width
  g.left = 0;
  g.top = CJK_TOP;
  g.dataLength = 0;   // unused for CJK glyphs
  g.dataOffset = cp;  // carry cp so the blitter can fetch the PROGMEM bitmap
  return &g;
}

// Vertical-metric floors for UI fonts that opt into the CJK fallback. Reader fonts
// must always pass cjkUiFallback=false so paginated text keeps its tight line-height
// (changing it would re-paginate every book and invalidate the .crosspoint/ cache).
// Each floor is independent — getFontAscenderSize floors the ascender to CJK_TOP (16),
// getLineHeight floors line-height to CJK_MIN_LINE_HEIGHT (24), getTextHeight floors
// height to CJK_UI_FONT_HEIGHT (20). The three constants differ on purpose; do not
// collapse them.
inline int ascenderFloor(int nativeAscender, bool cjkUiFallback) {
  return cjkUiFallback ? std::max(nativeAscender, static_cast<int>(CJK_TOP)) : nativeAscender;
}
inline int lineHeightFloor(int nativeAdvanceY, bool cjkUiFallback) {
  return cjkUiFallback ? std::max(nativeAdvanceY, static_cast<int>(CJK_MIN_LINE_HEIGHT)) : nativeAdvanceY;
}
inline int textHeightFloor(int nativeAscender, bool cjkUiFallback) {
  return cjkUiFallback ? std::max(nativeAscender, static_cast<int>(CjkUiFont20::CJK_UI_FONT_HEIGHT)) : nativeAscender;
}

inline bool isSynthesized(const EpdGlyph* g) {
  // Relational comparison of pointers into *different* array objects is UB, so compare
  // the converted integer addresses (well-defined integer comparison on a flat address space).
  if (g == nullptr) return false;
  const auto p = reinterpret_cast<uintptr_t>(g);
  return p >= reinterpret_cast<uintptr_t>(&g_ring[0]) && p < reinterpret_cast<uintptr_t>(&g_ring[RING]);
}

}  // namespace CjkUiFallback
