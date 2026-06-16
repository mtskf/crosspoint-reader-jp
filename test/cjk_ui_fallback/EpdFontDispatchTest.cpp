#include <gtest/gtest.h>

#include "CjkUiFallback.h"
#include "EpdFont.h"

namespace {
constexpr uint32_t KANJI = 0x65E5;                        // 日
const EpdGlyph kGlyphA{8, 12, 8 << 4, 0, 12, 0, 0};       // width,height,advanceX,left,top,dataLength,dataOffset
const EpdUnicodeInterval kIntervals[] = {{'A', 'A', 0}};  // font covers only 'A'
const uint8_t kBitmap[16] = {0};
EpdFontData makeFakeData() {
  EpdFontData d{};  // zero-init every field (no groups/kern/ligature/handler)
  d.bitmap = kBitmap;
  d.glyph = &kGlyphA;
  d.intervals = kIntervals;
  d.intervalCount = 1;  // must be >0 so getGlyph reaches the CJK branch
  d.advanceY = 14;
  d.ascender = 12;
  d.descender = 2;
  d.is2Bit = false;
  return d;
}
}  // namespace

TEST(EpdFontDispatch, FlagOffNoSynthesis) {
  EpdFontData d = makeFakeData();
  EpdFont f(&d);  // flag off by default
  EXPECT_FALSE(CjkUiFallback::isSynthesized(f.getGlyph(KANJI)));
}
TEST(EpdFontDispatch, FlagOnCjkSynthesized) {
  EpdFontData d = makeFakeData();
  EpdFont f(&d);
  f.enableCjkUiFallback();
  const EpdGlyph* g = f.getGlyph(KANJI);
  ASSERT_NE(g, nullptr);
  EXPECT_TRUE(CjkUiFallback::isSynthesized(g));
}
TEST(EpdFontDispatch, FlagOnAsciiNotSynthesized) {
  EpdFontData d = makeFakeData();
  EpdFont f(&d);
  f.enableCjkUiFallback();
  EXPECT_FALSE(CjkUiFallback::isSynthesized(f.getGlyph('A')));  // present -> real glyph
  EXPECT_FALSE(CjkUiFallback::isSynthesized(f.getGlyph('z')));  // absent, not CJK -> not synthesized
}
TEST(EpdFontDispatch, MeasuredWidthEqualsDrawAdvance) {
  EpdFontData d = makeFakeData();
  EpdFont f(&d);
  f.enableCjkUiFallback();
  int w = 0, h = 0;
  f.getTextDimensions("日日日", &w, &h);                          // getTextDimensions = maxX - minX
  EXPECT_EQ(w, 3 * (int)CjkUiFont20::getCjkUiGlyphWidth(KANJI));  // bbox width == 3 × advance
}
