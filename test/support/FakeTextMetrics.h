#pragma once

#include <ITextMetrics.h>
#include <Utf8.h>

#include <cstdint>
#include <string>
#include <vector>

// Deterministic: every codepoint advances cellPx, a space advances cellPx,
// kerning is zero. Makes line-break math exact and font-independent.
class FakeTextMetrics : public ITextMetrics {
 public:
  explicit FakeTextMetrics(int cellPx = 10) : cellPx_(cellPx) {}

  int getSpaceWidth(int, EpdFontFamily::Style) const override { return cellPx_; }
  int getSpaceAdvance(int, uint32_t, uint32_t, EpdFontFamily::Style) const override { return cellPx_; }
  int getKerning(int, uint32_t, uint32_t, EpdFontFamily::Style) const override { return 0; }
  int getTextAdvanceX(int, const char* text, EpdFontFamily::Style) const override {
    int width = 0;
    const auto* p = reinterpret_cast<const unsigned char*>(text);
    while (utf8NextCodepoint(&p) != 0) width += cellPx_;
    return width;
  }
  bool isSdCardFont(int) const override { return false; }
  void ensureSdCardFontReady(int, const char*, uint8_t) const override {}
  void ensureSdCardFontReady(int, const std::vector<std::string>&, bool, uint8_t) const override {}

 private:
  int cellPx_;
};
