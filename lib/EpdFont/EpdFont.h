#pragma once
#include "EpdFontData.h"

class EpdFont {
  void getTextBounds(const char* string, int startX, int startY, int* minX, int* minY, int* maxX, int* maxY) const;

 public:
  const EpdFontData* data;
  explicit EpdFont(const EpdFontData* data) : data(data) {}
  ~EpdFont() = default;

  // When enabled (UI fonts only), getGlyph() falls back to the built-in 20px CJK
  // UI font for true-CJK codepoints the primary font lacks. Reader fonts leave this off.
  void enableCjkUiFallback() { cjkUiFallback_ = true; }
  bool cjkUiFallbackEnabled() const { return cjkUiFallback_; }

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

 private:
  bool cjkUiFallback_ = false;
};
