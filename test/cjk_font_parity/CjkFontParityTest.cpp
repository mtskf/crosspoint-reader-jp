// CjkFontParityTest.cpp
// Verifies that every covered codepoint in cjk_ui_20 decodes to the same pixel
// pattern as the fork's cjk_ui_font_20.h (3-byte-row MSB-first source).
// ALL 3420 codepoints are checked — no early exit, no sampling.
//
// Host build: PROGMEM is a no-op (test/support/pgmspace.h), so all PROGMEM
// arrays reside in regular RAM — byte access works without pgm_read_byte.
//
// The fork reference arrays (kForkRef_*) come from cjk_ui_20_parity_ref.h, which
// is generated at build time by scripts/extract_cjk_fork_reference.py using the
// same parser as the converter (see CMakeLists.txt). This avoids include-shimming
// the fork header (namespace / per-glyph-comment quirks).

#include <gtest/gtest.h>

#include <cstdint>

#include "EpdFontData.h"
#include "cjk_ui_20.h"
#include "cjk_ui_20_parity_ref.h"

// Source format constants (fork)
static constexpr int FORK_HEIGHT = 20;
static constexpr int FORK_BPR = 3;   // bytes per row (24 bits, 20 used)
static constexpr int FORK_BPC = 60;  // bytes per char

// Decode a pixel from the fork's 3-byte-row MSB-first format.
// glyph_idx: index into kForkRef_Codepoints; x,y: pixel coordinates (0-based)
// width: kForkRef_Widths[glyph_idx]
static bool forkPixel(const uint8_t* fork_glyphs, int glyph_idx, int x, int y) {
  const int byte_idx = glyph_idx * FORK_BPC + y * FORK_BPR + (x >> 3);
  return (fork_glyphs[byte_idx] >> (7 - (x & 7))) & 1;
}

// Decode a pixel from the EpdFontData tight 1-bit packed format.
// glyph: pointer to EpdGlyph; bitmap: pointer to the full font bitmap array
static bool epdPixel(const uint8_t* bitmap, const EpdGlyph& glyph, int x, int y) {
  const int pos = y * glyph.width + x;
  const int byte_idx = static_cast<int>(glyph.dataOffset) + (pos >> 3);
  return (bitmap[byte_idx] >> (7 - (pos & 7))) & 1;
}

// Find glyph index in the EpdFontData for a given codepoint. Returns -1 if not found.
static int findEpdGlyphIndex(const EpdFontData& fd, uint32_t cp) {
  for (uint32_t i = 0; i < fd.intervalCount; i++) {
    const EpdUnicodeInterval& iv = fd.intervals[i];
    if (cp >= iv.first && cp <= iv.last) {
      return static_cast<int>(iv.offset + (cp - iv.first));
    }
  }
  return -1;
}

TEST(CjkFontParity, MetricFloor) {
  // Verify the EpdFontData carries the required metric floor values
  EXPECT_EQ(cjk_ui_20_font_data.ascender, 16);
  EXPECT_EQ(cjk_ui_20_font_data.advanceY, 24);
  EXPECT_EQ(cjk_ui_20_font_data.descender, 4);
  EXPECT_FALSE(cjk_ui_20_font_data.is2Bit);
  EXPECT_EQ(cjk_ui_20_font_data.glyphMissHandler, nullptr);
}

TEST(CjkFontParity, GlyphCount) {
  // Total glyphs in the EpdFontData must match the fork source count
  uint32_t total = 0;
  for (uint32_t i = 0; i < cjk_ui_20_font_data.intervalCount; i++) {
    const auto& iv = cjk_ui_20_font_data.intervals[i];
    total += iv.last - iv.first + 1;
  }
  EXPECT_EQ(total, CJK_UI_FONT_GLYPH_COUNT_VAL)
      << "EpdFontData covers " << total << " codepoints but fork has " << CJK_UI_FONT_GLYPH_COUNT_VAL;
  // The generated header's count constant must agree too.
  EXPECT_EQ(CJK_UI_20_GLYPH_COUNT, CJK_UI_FONT_GLYPH_COUNT_VAL);
}

TEST(CjkFontParity, AllCodepointsPixelPerfect) {
  // For EVERY covered codepoint, the EpdFontData bitmap must decode to the
  // same pixel matrix as the fork source. No early exit.
  int failures = 0;
  const int n = static_cast<int>(CJK_UI_FONT_GLYPH_COUNT_VAL);

  for (int i = 0; i < n; i++) {
    const uint32_t cp = kForkRef_Codepoints[i];
    const int w = kForkRef_Widths[i];

    const int epd_idx = findEpdGlyphIndex(cjk_ui_20_font_data, cp);
    ASSERT_GE(epd_idx, 0) << "codepoint U+" << std::hex << cp << " not found in EpdFontData";

    const EpdGlyph& g = cjk_ui_20_font_data.glyph[epd_idx];
    ASSERT_EQ(g.width, static_cast<uint8_t>(w)) << "U+" << std::hex << cp << " width mismatch";

    for (int y = 0; y < FORK_HEIGHT; y++) {
      for (int x = 0; x < w; x++) {
        const bool fork_bit = forkPixel(kForkRef_Glyphs, i, x, y);
        const bool epd_bit = epdPixel(cjk_ui_20_font_data.bitmap, g, x, y);
        if (fork_bit != epd_bit) {
          failures++;
          if (failures <= 5) {
            ADD_FAILURE() << "U+" << std::hex << cp << " pixel(" << std::dec << x << "," << y
                          << "): fork=" << fork_bit << " epd=" << epd_bit;
          }
        }
      }
    }
  }
  EXPECT_EQ(failures, 0) << "Total pixel mismatches: " << failures;
}

TEST(CjkFontParity, AdvanceXIsWidth12p4) {
  // For every glyph, advanceX == width << 4 (12.4 fixed-point)
  for (uint32_t i = 0; i < cjk_ui_20_font_data.intervalCount; i++) {
    const auto& iv = cjk_ui_20_font_data.intervals[i];
    for (uint32_t j = 0; j <= iv.last - iv.first; j++) {
      const EpdGlyph& g = cjk_ui_20_font_data.glyph[iv.offset + j];
      EXPECT_EQ(g.advanceX, static_cast<uint16_t>(g.width) << 4) << "U+" << std::hex << (iv.first + j);
    }
  }
}
