// CjkFontSelfTest.cpp
// Fork-INDEPENDENT structural invariants of the committed cjk_ui_20 artifact.
// Built and run unconditionally (incl. CI), so the generated EpdFontData asset's
// metrics, per-glyph metadata, bitmap-offset contiguity, and interval table are
// guarded on every build — even where the fork source (needed for full pixel
// parity, see CjkFontParityTest) is absent.

#include <gtest/gtest.h>

#include <cstdint>

#include "EpdFontData.h"
#include "cjk_ui_20.h"

static constexpr int CJK_HEIGHT = 20;

TEST(CjkFontSelf, MetricFloor) {
  EXPECT_EQ(cjk_ui_20_font_data.ascender, 16);
  EXPECT_EQ(cjk_ui_20_font_data.advanceY, 24);
  EXPECT_EQ(cjk_ui_20_font_data.descender, 4);
  EXPECT_FALSE(cjk_ui_20_font_data.is2Bit);
  EXPECT_EQ(cjk_ui_20_font_data.glyphMissHandler, nullptr);
  EXPECT_EQ(cjk_ui_20_font_data.glyphMissCtx, nullptr);
}

TEST(CjkFontSelf, GlyphCountMatchesHeaderConstant) {
  uint32_t total = 0;
  for (uint32_t i = 0; i < cjk_ui_20_font_data.intervalCount; i++) {
    const auto& iv = cjk_ui_20_font_data.intervals[i];
    total += iv.last - iv.first + 1;
  }
  EXPECT_EQ(total, CJK_UI_20_GLYPH_COUNT);
}

TEST(CjkFontSelf, GlyphMetadataConsistent) {
  // Every glyph carries the fixed bitmap geometry the converter promises, and
  // advanceX / dataLength derive from width exactly. Pixel-only parity cannot
  // catch a regression in these metadata fields.
  for (uint32_t i = 0; i < CJK_UI_20_GLYPH_COUNT; i++) {
    const EpdGlyph& g = cjk_ui_20_font_data.glyph[i];
    EXPECT_EQ(g.height, static_cast<uint8_t>(CJK_HEIGHT)) << "glyph " << i << " height";
    EXPECT_EQ(g.top, static_cast<int16_t>(16)) << "glyph " << i << " top";
    EXPECT_EQ(g.left, static_cast<int16_t>(0)) << "glyph " << i << " left";
    EXPECT_GE(g.width, 1) << "glyph " << i << " width";
    EXPECT_LE(g.width, CJK_HEIGHT) << "glyph " << i << " width";
    EXPECT_EQ(g.advanceX, static_cast<uint16_t>(g.width) << 4) << "glyph " << i << " advanceX";
    const int expected_dl = (g.width * CJK_HEIGHT + 7) / 8;
    EXPECT_EQ(g.dataLength, static_cast<uint16_t>(expected_dl)) << "glyph " << i << " dataLength";
  }
}

TEST(CjkFontSelf, DataOffsetContiguous) {
  // Bitmap offsets must accumulate by dataLength with no gaps or overlaps.
  uint32_t expected = 0;
  for (uint32_t i = 0; i < CJK_UI_20_GLYPH_COUNT; i++) {
    const EpdGlyph& g = cjk_ui_20_font_data.glyph[i];
    EXPECT_EQ(g.dataOffset, expected) << "glyph " << i << " dataOffset";
    expected += g.dataLength;
  }
}

TEST(CjkFontSelf, IntervalsSortedNonOverlappingContiguous) {
  // findGlyph relies on sorted, non-overlapping intervals whose offsets march in
  // step with the codepoints they cover.
  ASSERT_GT(cjk_ui_20_font_data.intervalCount, 0u);
  EXPECT_EQ(cjk_ui_20_font_data.intervals[0].offset, 0u);
  for (uint32_t i = 1; i < cjk_ui_20_font_data.intervalCount; i++) {
    const auto& prev = cjk_ui_20_font_data.intervals[i - 1];
    const auto& cur = cjk_ui_20_font_data.intervals[i];
    EXPECT_LT(prev.last, cur.first) << "intervals[" << (i - 1) << "]/[" << i << "] overlap or unsorted";
    EXPECT_EQ(cur.offset, prev.offset + (prev.last - prev.first + 1)) << "offset discontinuity at " << i;
  }
}
