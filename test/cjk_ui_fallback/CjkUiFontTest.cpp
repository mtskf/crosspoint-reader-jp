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
