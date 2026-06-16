#include <gtest/gtest.h>

#include "cjk_ui_font_20.h"

TEST(CjkUiFont, CoversHiraganaKanjiAndAscii) {
  EXPECT_TRUE(CjkUiFont20::hasCjkUiGlyph(0x3042));  // あ
  EXPECT_TRUE(CjkUiFont20::hasCjkUiGlyph(0x65E5));  // 日
  // The font intentionally contains ASCII too; ASCII is excluded from the fallback by
  // CjkUiFallback::shouldUse() (tested below), NOT by hasCjkUiGlyph().
  EXPECT_TRUE(CjkUiFont20::hasCjkUiGlyph('A'));
  EXPECT_FALSE(CjkUiFont20::hasCjkUiGlyph(0x10FFFF));  // out of BMP -> not covered
}

TEST(CjkUiFont, WidthAndBitmapNonNullForCovered) {
  EXPECT_GT(CjkUiFont20::getCjkUiGlyphWidth(0x3042), 0);
  EXPECT_NE(CjkUiFont20::getCjkUiGlyph(0x3042), nullptr);
}

#include "CjkUiFallback.h"

TEST(CjkUiFallback, GateAcceptsCjkRejectsAscii) {
  EXPECT_TRUE(CjkUiFallback::shouldUse(0x3042));  // あ → CJK range + covered
  EXPECT_FALSE(CjkUiFallback::shouldUse('A'));    // ASCII never
  EXPECT_FALSE(CjkUiFallback::shouldUse(0x0041));
}

TEST(CjkUiFallback, SynthesizedGlyphMetrics) {
  const EpdGlyph* g = CjkUiFallback::makeGlyph(0x3042);
  ASSERT_NE(g, nullptr);
  EXPECT_TRUE(CjkUiFallback::isSynthesized(g));
  EXPECT_EQ(g->dataOffset, 0x3042u);  // cp carried for bitmap fetch
  EXPECT_EQ(g->height, 20);
  EXPECT_EQ(g->left, 0);
  EXPECT_EQ(g->advanceX >> 4, g->width);  // advance == bbox width -> measure==draw
  EXPECT_EQ(g->width, CjkUiFont20::getCjkUiGlyphWidth(0x3042));
  EXPECT_FALSE(CjkUiFallback::isSynthesized(nullptr));
}

// The codepoint array drives a binary search (findGlyphIndex). A regenerator that
// emits an unsorted array would silently mis-identify glyphs at lookup time.
TEST(CjkUiFont, CodepointArrayIsStrictlyAscending) {
  for (uint16_t i = 1; i < CjkUiFont20::CJK_UI_FONT_GLYPH_COUNT; ++i) {
    const uint16_t prev = CjkUiFont20::CJK_UI_CODEPOINTS[i - 1];
    const uint16_t curr = CjkUiFont20::CJK_UI_CODEPOINTS[i];
    ASSERT_LT(prev, curr) << "unsorted at idx " << i << " (0x" << std::hex << prev << " >= 0x" << curr << ")";
  }
}

// `g.advanceX = w << 4` derives advance from glyph width; oversized widths would
// overflow uint16_t (max w * 16 = 4080 fits 65535, but only because w <= 255).
TEST(CjkUiFont, AllWidthsInRange) {
  for (uint16_t i = 0; i < CjkUiFont20::CJK_UI_FONT_GLYPH_COUNT; ++i) {
    const uint8_t w = CjkUiFont20::CJK_UI_GLYPH_WIDTHS[i];
    ASSERT_GT(w, 0) << "zero-width glyph at idx " << i;
    ASSERT_LE(w, CjkUiFont20::CJK_UI_FONT_WIDTH) << "glyph at idx " << i << " exceeds storage box";
  }
}

// The "measure == draw" invariant is per-codepoint (proportional widths), so pin it
// across a sample of the width distribution rather than one cp like the original
// SynthesizedGlyphMetrics test.
TEST(CjkUiFallback, SynthesizedAdvanceEqualsWidthForCoveredGlyphs) {
  const uint32_t cps[] = {
      'A',     // ASCII (proportional, narrow)
      0x3042,  // hiragana あ (wide)
      0x65E5,  // common kanji 日 (wide)
      0x2014,  // em-dash (full-width punctuation)
      0x300C,  // CJK punctuation 「
  };
  for (uint32_t cp : cps) {
    if (!CjkUiFont20::hasCjkUiGlyph(cp)) continue;
    const EpdGlyph* g = CjkUiFallback::makeGlyph(cp);
    ASSERT_NE(g, nullptr) << "cp 0x" << std::hex << cp;
    EXPECT_GT(g->width, 0) << "cp 0x" << std::hex << cp;
    EXPECT_LE(g->width, CjkUiFont20::CJK_UI_FONT_WIDTH) << "cp 0x" << std::hex << cp;
    EXPECT_EQ(g->advanceX, static_cast<uint16_t>(g->width) << 4)
        << "measure==draw invariant broken at cp 0x" << std::hex << cp;
    EXPECT_EQ(g->width, CjkUiFont20::getCjkUiGlyphWidth(cp))
        << "synthesized width disagrees with font width at cp 0x" << std::hex << cp;
  }
}

// The metric helpers in CjkUiFallback are pulled out for testability — pin the three
// independent floors so a copy-paste refactor cannot conflate them.
TEST(CjkUiFallbackMetrics, FlagOffNeverClamps) {
  // Reader fonts must NEVER inherit the CJK floor — would re-paginate every book.
  EXPECT_EQ(CjkUiFallback::ascenderFloor(8, false), 8);
  EXPECT_EQ(CjkUiFallback::lineHeightFloor(14, false), 14);
  EXPECT_EQ(CjkUiFallback::textHeightFloor(8, false), 8);
}
TEST(CjkUiFallbackMetrics, AscenderClampsUpWhenCjkAndSmall) {
  EXPECT_EQ(CjkUiFallback::ascenderFloor(8, true), static_cast<int>(CjkUiFallback::CJK_TOP));  // 16
}
TEST(CjkUiFallbackMetrics, AscenderUnchangedWhenCjkAndLarger) {
  EXPECT_EQ(CjkUiFallback::ascenderFloor(24, true), 24);  // no regression for large UI fonts
}
TEST(CjkUiFallbackMetrics, EachMetricUsesItsOwnFloor) {
  // Pin the three different floor constants so a copy-paste refactor can't conflate them.
  EXPECT_EQ(CjkUiFallback::ascenderFloor(0, true), 16);    // CJK_TOP
  EXPECT_EQ(CjkUiFallback::lineHeightFloor(0, true), 24);  // CJK_MIN_LINE_HEIGHT
  EXPECT_EQ(CjkUiFallback::textHeightFloor(0, true), 20);  // CJK_UI_FONT_HEIGHT
}
