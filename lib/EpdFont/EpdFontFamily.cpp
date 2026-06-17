#include "EpdFontFamily.h"
// REPLACEMENT_GLYPH (0xFFFD) is provided by Utf8.h as a macro — do NOT redeclare it here.
#include <Utf8.h>

#include <algorithm>

const EpdFont* EpdFontFamily::getFont(const Style style) const {
  // Extract font style bits (ignore UNDERLINE bit for font selection)
  const bool hasBold = (style & BOLD) != 0;
  const bool hasItalic = (style & ITALIC) != 0;

  if (hasBold && hasItalic) {
    if (boldItalic) return boldItalic;
    if (bold) return bold;
    if (italic) return italic;
  } else if (hasBold && bold) {
    return bold;
  } else if (hasItalic && italic) {
    return italic;
  }

  return regular;
}

const EpdFontData* EpdFontFamily::getData(const Style style) const { return getFont(style)->data; }

const EpdGlyph* EpdFontFamily::getGlyph(const uint32_t cp, const Style style) const {
  return getFont(style)->getGlyph(cp);
}

int8_t EpdFontFamily::getKerning(const uint32_t leftCp, const uint32_t rightCp, const Style style) const {
  return getFont(style)->getKerning(leftCp, rightCp);
}

uint32_t EpdFontFamily::applyLigatures(const uint32_t cp, const char*& text, const Style style) const {
  return getFont(style)->applyLigatures(cp, text);
}

ResolvedGlyph EpdFontFamily::resolveGlyph(const uint32_t cp, const Style style) const {
  const EpdFont* primary = getFont(style);

  // Fast path: no fallback registered (reader body-text fonts, SD card fonts).
  // Skip the pointer-identity miss detection entirely — these paths may use
  // glyphMissHandler with ring-buffer-backed glyph storage where calling
  // getGlyph(REPLACEMENT_GLYPH) before getGlyph(cp) would invalidate state.
  if (fallback_ == nullptr) {
    return {primary->getGlyph(cp), primary->data};
  }

  // Family with fallback: pointer-identity miss detection is safe HERE because
  // fallback is only registered on UI fonts (built-in static tables, no
  // glyphMissHandler), so getGlyph(REPLACEMENT_GLYPH) returns a stable pointer
  // that the subsequent getGlyph(cp) cannot invalidate. setFallback() asserts
  // this precondition (glyphMissHandler == nullptr) for both fonts.
  const EpdGlyph* replacement = primary->getGlyph(REPLACEMENT_GLYPH);
  const EpdGlyph* g = primary->getGlyph(cp);

  // A primary hit: non-null AND (not the replacement, OR cp IS the replacement itself)
  const bool primaryHit = g && (g != replacement || cp == REPLACEMENT_GLYPH);
  if (primaryHit) {
    return {g, primary->data};
  }

  const EpdGlyph* fallbackReplacement = fallback_->getGlyph(REPLACEMENT_GLYPH);
  const EpdGlyph* fg = fallback_->getGlyph(cp);
  const bool fallbackHit = fg && (fg != fallbackReplacement || cp == REPLACEMENT_GLYPH);
  if (fallbackHit) {
    return {fg, fallback_->data};
  }

  // Both missed — return primary's replacement glyph with primary's data
  return {replacement ? replacement : g, primary->data};
}

int EpdFontFamily::getMaxAscender(const Style style) const {
  const int primary = getFont(style)->data->ascender;
  if (fallback_) return std::max(primary, fallback_->data->ascender);
  return primary;
}

int EpdFontFamily::getMaxAdvanceY(const Style style) const {
  const int primary = static_cast<int>(getFont(style)->data->advanceY);
  if (fallback_) return std::max(primary, static_cast<int>(fallback_->data->advanceY));
  return primary;
}
