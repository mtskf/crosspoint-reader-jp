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

// A CJK-range codepoint NOT covered by the fallback font (e.g. CJK Ext A U+3400)
// MUST NOT synthesize from an empty bitmap: shouldUse() must reject it via the
// hasCjkUiGlyph() arm. Today the dispatch falls through to getGlyph(REPLACEMENT_GLYPH)
// which itself returns nullptr because the fake font has no replacement glyph.
TEST(EpdFontDispatch, FlagOnCjkOutsideFallbackTableNotSynthesized) {
  ASSERT_FALSE(CjkUiFont20::hasCjkUiGlyph(0x3400)) << "test premise: pick an uncovered CJK cp";
  EpdFontData d = makeFakeData();
  EpdFont f(&d);
  f.enableCjkUiFallback();
  const EpdGlyph* g = f.getGlyph(0x3400);
  EXPECT_FALSE(CjkUiFallback::isSynthesized(g));
  EXPECT_EQ(g, nullptr);
}

// Mixed-script UI strings ("Battery 残り 80%") are the real workload. Pin that width
// sums across the CJK / ASCII boundary.
TEST(EpdFontDispatch, MixedAsciiCjkWidthAccountsForBoth) {
  EpdFontData d = makeFakeData();
  EpdFont f(&d);
  f.enableCjkUiFallback();
  int wA = 0, wK = 0, wMix = 0, h = 0;
  f.getTextDimensions("A", &wA, &h);
  f.getTextDimensions("日", &wK, &h);
  f.getTextDimensions("A日", &wMix, &h);
  // Allow ±1 for differential rounding at the script boundary but pin the budget.
  EXPECT_NEAR(wMix, wA + wK, 1) << "mixed-script width drifted from sum of parts";
}

// Existing UI strings must not re-flow when the CJK flag is toggled on a font:
// a regression here would re-paginate every cached page on update.
TEST(EpdFontDispatch, AsciiOnlyWithFlagOnDoesNotRegress) {
  EpdFontData d = makeFakeData();
  EpdFont fOff(&d);
  EpdFont fOn(&d);
  fOn.enableCjkUiFallback();
  int wOff = 0, wOn = 0, h = 0;
  fOff.getTextDimensions("AAA", &wOff, &h);
  fOn.getTextDimensions("AAA", &wOn, &h);
  EXPECT_EQ(wOff, wOn) << "CJK flag must not affect pure-ASCII measurement";
}

// Measurement strings longer than RING must still measure correctly. If a future
// caller held a glyph pointer across iterations, this would drift.
TEST(EpdFontDispatch, MeasurementLongerThanRingStaysCorrect) {
  EpdFontData d = makeFakeData();
  EpdFont f(&d);
  f.enableCjkUiFallback();
  int w = 0, h = 0;
  // 16 kanji = 2 ring wraps (RING = 8).
  f.getTextDimensions("日日日日日日日日日日日日日日日日", &w, &h);
  EXPECT_EQ(w, 16 * (int)CjkUiFont20::getCjkUiGlyphWidth(KANJI));
}

// Ring eviction semantics: a slot returned RING-1 calls ago must still be intact;
// the next call wraps and overwrites slot 0. Callers MUST consume the pointer before
// the (RING)th subsequent makeGlyph call.
TEST(CjkUiFallbackRing, EvictsAfterRingCalls) {
  const EpdGlyph* g0 = CjkUiFallback::makeGlyph(0x3042);  // あ
  ASSERT_NE(g0, nullptr);
  ASSERT_EQ(g0->dataOffset, 0x3042u);
  // Fill the ring with RING-1 other glyphs. g0 must still be intact.
  for (int i = 0; i < CjkUiFallback::RING - 1; ++i) {
    CjkUiFallback::makeGlyph(0x3044);  // い
  }
  EXPECT_EQ(g0->dataOffset, 0x3042u) << "evicted too early";
  // One more call wraps the ring back to slot 0 — g0 is now overwritten.
  CjkUiFallback::makeGlyph(0x3046);  // う
  EXPECT_EQ(g0->dataOffset, 0x3046u) << "ring eviction broke; callers MUST consume before next makeGlyph";
}
