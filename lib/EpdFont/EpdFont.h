#pragma once
#include "EpdFontData.h"

class EpdFontFamily;  // forward declaration — no #include (would be circular)

class EpdFont {
 public:
  const EpdFontData* data;
  explicit EpdFont(const EpdFontData* data) : data(data) {}
  ~EpdFont() = default;

  // Public surface unchanged for single-font callers: measures with this font only.
  void getTextBounds(const char* string, int startX, int startY, int* minX, int* minY, int* maxX, int* maxY) const {
    getTextBoundsImpl(string, startX, startY, minX, minY, maxX, maxY, nullptr, /*style=*/0);  // 0 = REGULAR
  }

  // Family-aware variant used by EpdFontFamily::getTextBounds. When `family` is non-null,
  // glyph lookup goes through family->resolveGlyph (primary + fallback chain) so CJK
  // codepoints measure with the fallback font instead of the replacement box.
  // `style` is passed as int to avoid pulling EpdFontFamily.h into this header;
  // EpdFont.cpp casts it back to EpdFontFamily::Style at the resolveGlyph call site.
  void getTextBoundsImpl(const char* string, int startX, int startY, int* minX, int* minY, int* maxX, int* maxY,
                         const EpdFontFamily* family, int style) const;

  void getTextDimensions(const char* string, int* w, int* h) const;

  const EpdGlyph* getGlyph(uint32_t cp) const;

  /// Returns the kerning adjustment (4.4 fixed-point in pixels) between two codepoints.
  /// Returns 0 if no kerning data exists for the pair.
  int8_t getKerning(uint32_t leftCp, uint32_t rightCp) const;

  /// Returns the ligature codepoint for a pair, or 0 if no ligature exists.
  uint32_t getLigature(uint32_t leftCp, uint32_t rightCp) const;

  /// Greedily applies ligature substitutions starting from cp, consuming
  /// as many following codepoints from text as possible. Returns the
  /// (possibly substituted) codepoint; advances text past consumed chars.
  uint32_t applyLigatures(uint32_t cp, const char*& text) const;
};
