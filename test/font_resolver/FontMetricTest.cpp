#include <gtest/gtest.h>
#include "EpdFontFamily.h"

namespace {
const EpdGlyph kDummyGlyph = {};

EpdFontData makeFont(int advY, int asc, int desc) {
    static const EpdUnicodeInterval kIv[] = {{'A', 'A', 0}};
    EpdFontData d{};
    d.glyph = &kDummyGlyph;
    d.intervals = kIv; d.intervalCount = 1;
    d.advanceY = static_cast<uint8_t>(advY);
    d.ascender = asc; d.descender = desc;
    d.is2Bit = false;
    return d;
}
}  // namespace

TEST(FontMetric, LineHeightMaxesOverFallback) {
    EpdFontData primary = makeFont(14, 10, 2);   // primary: advanceY=14
    EpdFontData fallback = makeFont(24, 16, 4);  // fallback (CJK): advanceY=24
    EpdFont pf(&primary), ff(&fallback);
    EpdFontFamily fam(&pf);
    fam.setFallback(&ff);
    EXPECT_EQ(fam.getMaxAdvanceY(), 24);  // max(14, 24) == 24
}

TEST(FontMetric, LineHeightFallsBackToPrimaryWhenNoFallback) {
    EpdFontData primary = makeFont(14, 10, 2);
    EpdFont pf(&primary);
    EpdFontFamily fam(&pf);
    EXPECT_EQ(fam.getMaxAdvanceY(), 14);  // no fallback → primary only
}

TEST(FontMetric, AscenderMaxesOverFallback) {
    EpdFontData primary = makeFont(14, 10, 2);
    EpdFontData fallback = makeFont(24, 16, 4);
    EpdFont pf(&primary), ff(&fallback);
    EpdFontFamily fam(&pf);
    fam.setFallback(&ff);
    EXPECT_EQ(fam.getMaxAscender(), 16);  // max(10, 16) == 16
}

TEST(FontMetric, AscenderUnchangedWhenFallbackSmaller) {
    EpdFontData primary = makeFont(20, 18, 2);   // ubuntu_12 has ascender 18
    EpdFontData fallback = makeFont(24, 16, 4);  // CJK: ascender 16 < 18
    EpdFont pf(&primary), ff(&fallback);
    EpdFontFamily fam(&pf);
    fam.setFallback(&ff);
    EXPECT_EQ(fam.getMaxAscender(), 18);  // max(18, 16) == 18; primary wins
}
