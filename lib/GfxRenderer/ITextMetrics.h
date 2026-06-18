#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <EpdFontFamily.h>

// Text-measurement surface used by layout code (ParsedText). Extracted from
// GfxRenderer so layout has no dependency on the E-ink HAL / Arduino, which
// keeps it host-testable. GfxRenderer implements this interface; production
// code keeps passing a GfxRenderer&, which up-casts implicitly.
class ITextMetrics {
 public:
  virtual ~ITextMetrics() = default;

  virtual int getSpaceWidth(int fontId, EpdFontFamily::Style style) const = 0;
  virtual int getSpaceAdvance(int fontId, uint32_t leftCp, uint32_t rightCp,
                              EpdFontFamily::Style style) const = 0;
  virtual int getKerning(int fontId, uint32_t leftCp, uint32_t rightCp,
                         EpdFontFamily::Style style) const = 0;
  virtual int getTextAdvanceX(int fontId, const char* text, EpdFontFamily::Style style) const = 0;
  virtual bool isSdCardFont(int fontId) const = 0;
  virtual void ensureSdCardFontReady(int fontId, const char* utf8Text, uint8_t styleMask) const = 0;
  virtual void ensureSdCardFontReady(int fontId, const std::vector<std::string>& words, bool includeHyphen,
                                     uint8_t styleMask) const = 0;
};
