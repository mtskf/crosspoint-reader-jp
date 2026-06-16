#include <gtest/gtest.h>
#include "cjk_ui_font_20.h"

TEST(CjkUiFont, CoversHiraganaKanjiAndAscii) {
  EXPECT_TRUE(CjkUiFont20::hasCjkUiGlyph(0x3042));   // あ
  EXPECT_TRUE(CjkUiFont20::hasCjkUiGlyph(0x65E5));   // 日
  // The font intentionally contains ASCII too; ASCII is excluded from the fallback by
  // CjkUiFallback::shouldUse() (tested below), NOT by hasCjkUiGlyph().
  EXPECT_TRUE(CjkUiFont20::hasCjkUiGlyph('A'));
  EXPECT_FALSE(CjkUiFont20::hasCjkUiGlyph(0x10FFFF));// out of BMP -> not covered
}

TEST(CjkUiFont, WidthAndBitmapNonNullForCovered) {
  EXPECT_GT(CjkUiFont20::getCjkUiGlyphWidth(0x3042), 0);
  EXPECT_NE(CjkUiFont20::getCjkUiGlyph(0x3042), nullptr);
}

#include "CjkUiFallback.h"

TEST(CjkUiFallback, GateAcceptsCjkRejectsAscii) {
  EXPECT_TRUE(CjkUiFallback::shouldUse(0x3042));   // あ → CJK range + covered
  EXPECT_FALSE(CjkUiFallback::shouldUse('A'));     // ASCII never
  EXPECT_FALSE(CjkUiFallback::shouldUse(0x0041));
}

TEST(CjkUiFallback, SynthesizedGlyphMetrics) {
  const EpdGlyph* g = CjkUiFallback::makeGlyph(0x3042);
  ASSERT_NE(g, nullptr);
  EXPECT_TRUE(CjkUiFallback::isSynthesized(g));
  EXPECT_EQ(g->dataOffset, 0x3042u);               // cp carried for bitmap fetch
  EXPECT_EQ(g->height, 20);
  EXPECT_EQ(g->left, 0);
  EXPECT_EQ(g->advanceX >> 4, g->width);           // advance == bbox width -> measure==draw
  EXPECT_EQ(g->width, CjkUiFont20::getCjkUiGlyphWidth(0x3042));
  EXPECT_FALSE(CjkUiFallback::isSynthesized(nullptr));
}
