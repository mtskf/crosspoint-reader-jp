#include <gtest/gtest.h>
#include "EpdFontFamily.h"
#include "EpdFontData.h"
#include <Utf8.h>
#include <cstring>

// ---- Test fixture ----
// CRITICAL: EpdFont and EpdFontFamily have no default constructors — they require
// a const EpdFontData* argument. Use constructor-initializer-list (NOT SetUp()
// reassignment). Static data array members MUST be declared BEFORE the EpdFontData
// members that point into them — C++ initialises members top-down within a class.
//
// EpdFontData has many members between `groups` and `glyphMissHandler`, so
// positional aggregate initialization would mis-land nullptr on groupCount (uint16_t).
// Instead, use file-local by-value helpers (makePrimaryData / makeFallbackData)
// that set only the fields we need and leave the rest value-initialized.

static const EpdGlyph kGlyphA_arr[] = {
    {8, 10, uint16_t(8 << 4), 0, 10, 10, 0},  // 'A'
    {6,  8, uint16_t(6 << 4), 0,  8,  6, 0},  // U+FFFD replacement
};
static const EpdUnicodeInterval kIntervalsA[] = {
    {'A',    'A',    0},
    {0xFFFD, 0xFFFD, 1},
};
static const uint8_t kBitmapA[] = {0};  // blank glyphs OK for metric tests
static const EpdGlyph kGlyphCjk_arr[] = {
    {20, 20, uint16_t(20 << 4), 0, 16, 50, 0},  // U+65E5 日
};
static const EpdUnicodeInterval kIntervalsCjk[] = {
    {0x65E5, 0x65E5, 0},
};
static const uint8_t kBitmapCjk[] = {0};

static EpdFontData makePrimaryData() {
    EpdFontData d{};
    d.bitmap        = kBitmapA;
    d.glyph         = kGlyphA_arr;
    d.intervals     = kIntervalsA;
    d.intervalCount = 2;
    d.advanceY      = 14;
    d.ascender      = 10;
    d.descender     = 2;
    d.is2Bit        = false;
    // glyphMissHandler and all other fields remain nullptr/zero (value-initialized)
    return d;
}

static EpdFontData makeFallbackData() {
    EpdFontData d{};
    d.bitmap        = kBitmapCjk;
    d.glyph         = kGlyphCjk_arr;
    d.intervals     = kIntervalsCjk;
    d.intervalCount = 1;
    d.advanceY      = 24;
    d.ascender      = 16;
    d.descender     = 4;
    d.is2Bit        = false;
    // glyphMissHandler and all other fields remain nullptr/zero (value-initialized)
    return d;
}

class FontResolverTest : public ::testing::Test {
 protected:
  FontResolverTest()
      : primaryData_(makePrimaryData()),
        fallbackData_(makeFallbackData()),
        primaryFont_(&primaryData_),
        fallbackFont_(&fallbackData_),
        familyWithFallback_(&primaryFont_),
        familyNoFallback_(&primaryFont_) {
    familyWithFallback_.setFallback(&fallbackFont_);
  }

  EpdFontData primaryData_;
  EpdFontData fallbackData_;
  EpdFont primaryFont_;
  EpdFont fallbackFont_;
  EpdFontFamily familyWithFallback_;
  EpdFontFamily familyNoFallback_;
};

TEST_F(FontResolverTest, ResolveReturnsPrimaryForCoveredAscii) {
    auto r = familyWithFallback_.resolveGlyph('A');
    EXPECT_EQ(r.data, &primaryData_)  << "data must come from primary";
    EXPECT_EQ(r.glyph->width, 8u)     << "must be the 'A' glyph (width=8)";
}

TEST_F(FontResolverTest, ResolveFallsBackForCpOnlyInFallback) {
    auto r = familyWithFallback_.resolveGlyph(0x65E5u);  // 日 — not in primary
    EXPECT_EQ(r.data,  &fallbackData_) << "data must come from fallback for missed primary";
    EXPECT_EQ(r.glyph->width, 20u)     << "must be the CJK glyph (width=20)";
}

TEST_F(FontResolverTest, ResolveReturnsReplacementWhenNeitherCovers) {
    // cp not in primary OR fallback
    auto r = familyWithFallback_.resolveGlyph(0x9999u);
    // data must be from primary (fallback replacement would be wrong source)
    EXPECT_EQ(r.data, &primaryData_);
    // glyph must be primary's replacement (U+FFFD)
    EXPECT_EQ(r.glyph->width, 6u);  // replacement glyph width
}

TEST_F(FontResolverTest, ResolvedDataMatchesGlyphSource) {
    // The critical owner-data invariant: glyph and data are always co-sourced
    auto rA    = familyWithFallback_.resolveGlyph('A');
    auto rCJK  = familyWithFallback_.resolveGlyph(0x65E5u);
    auto rMiss = familyWithFallback_.resolveGlyph(0x9999u);

    // 'A': both from primary
    EXPECT_EQ(rA.data, primaryFont_.data);
    // 日: both from fallback
    EXPECT_EQ(rCJK.data, fallbackFont_.data);
    // miss: both from primary (replacement path)
    EXPECT_EQ(rMiss.data, primaryFont_.data);
}

TEST_F(FontResolverTest, NoFallbackMissBehavesLikePrimary) {
    auto r = familyNoFallback_.resolveGlyph(0x65E5u);
    EXPECT_EQ(r.data, &primaryData_);  // no fallback → primary's replacement
    EXPECT_EQ(r.glyph->width, 6u);    // U+FFFD width
}

TEST_F(FontResolverTest, ResolveReplacementGlyphItself) {
    // Resolving U+FFFD should always return the primary's replacement (hit, not miss)
    auto r = familyWithFallback_.resolveGlyph(0xFFFDu);
    EXPECT_EQ(r.data, &primaryData_);
}

TEST(FontResolver, MissingGlyphReturnsNullWithPrimaryData) {
    // Build a primary with no REPLACEMENT_GLYPH (bare font: only covers 'A', no U+FFFD)
    // and no fallback. resolveGlyph(cp not in font) must return {nullptr, primary->data}
    // rather than crashing. (B-4: ResolvedGlyph::glyph is nullable for bare fonts.)
    static const EpdGlyph kBareGlyph = {8, 10, uint16_t(8 << 4), 0, 10, 10, 0};
    static const EpdUnicodeInterval kBareIntervals[] = {{'A', 'A', 0}};
    static const uint8_t kBareBitmap[10] = {};
    EpdFontData bareData{};
    bareData.bitmap = kBareBitmap;
    bareData.glyph = &kBareGlyph;
    bareData.intervals = kBareIntervals; bareData.intervalCount = 1;
    bareData.advanceY = 14; bareData.ascender = 10; bareData.descender = 2;
    EpdFont bareFont(&bareData);
    EpdFontFamily bareFam(&bareFont);
    // No fallback registered.
    auto r = bareFam.resolveGlyph(0x9FFEu);
    EXPECT_EQ(r.data, &bareData)   << "data must always point to primary even for bare font";
    // glyph may be nullptr (bare font has no REPLACEMENT_GLYPH) — do NOT dereference without check.
    // The test simply asserts no crash occurs.
}
