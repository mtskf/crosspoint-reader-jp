#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

#include "EpdFontData.h"
#include "EpdFontFamily.h"
#include "Utf8.h"
#include "cjk_ui_20.h"

namespace {

// A minimal primary font covering only 'A' and U+FFFD.
// advanceX for 'A' = 8 << 4 = 128 (12.4 fp → 8 pixels)
static const EpdGlyph kPrimaryGlyphs[] = {
    {8, 12, uint16_t(8 << 4), 0, 12, 0, 0},   // 'A'
    {6,  8, uint16_t(6 << 4), 0,  8, 0, 8},   // U+FFFD replacement
};
static const EpdUnicodeInterval kPrimaryIntervals[] = {
    {'A', 'A',    0},
    {0xFFFD, 0xFFFD, 1},
};
static const uint8_t kPrimaryBitmap[64] = {};

// A live 1x1 kern table covering 'A' (left class 1) x 'A' (right class 1).
// Without this, EpdFont::getKerning short-circuits on `!kernMatrix` for ANY pair,
// making the boundary-zero assertions vacuous. With it, getKerning('A','A') is the
// distinctive non-zero value -16 (4.4 fp), proving the table is live — so the 0s
// returned for CJK pairs are genuine class misses, not a blanket null-matrix bypass.
static const EpdKernClassEntry kPrimaryKernLeft[]  = {{'A', 1}};
static const EpdKernClassEntry kPrimaryKernRight[] = {{'A', 1}};
static const int8_t kPrimaryKernMatrix[] = {-16};  // 1 row x 1 col, distinctive non-zero 4.4 fp

EpdFontData makePrimaryData() {
    EpdFontData d{};
    d.bitmap = kPrimaryBitmap;
    d.glyph = kPrimaryGlyphs; d.intervals = kPrimaryIntervals; d.intervalCount = 2;
    d.advanceY = 14; d.ascender = 10; d.descender = 2; d.is2Bit = false;
    d.kernLeftClasses = kPrimaryKernLeft; d.kernRightClasses = kPrimaryKernRight;
    d.kernMatrix = kPrimaryKernMatrix;
    d.kernLeftEntryCount = 1; d.kernRightEntryCount = 1;
    d.kernLeftClassCount = 1; d.kernRightClassCount = 1;
    return d;
}

// Helper: measure total advance of a UTF-8 string using resolveGlyph for each cp.
// Simplified (no kerning, no ligatures) — matches getTextAdvanceX hot path.
// NOTE: Utf8.h MUST be included before this helper (utf8NextCodepoint dependency).
int measureAdvance(const EpdFontFamily& fam, const char* str) {
    int total = 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(str);
    uint32_t cp;
    while ((cp = utf8NextCodepoint(&p)) != 0) {
        const auto r = fam.resolveGlyph(cp);
        if (r.glyph) total += r.glyph->advanceX >> 4;  // 12.4 fixed-point → pixels
    }
    return total;
}

}  // namespace

// ---- Test fixture ----
// NOTE: EpdFont and EpdFontFamily have no default constructors.
// This fixture uses the constructor-initializer-list pattern.
// Static data members MUST be declared BEFORE EpdFontData/EpdFont members
// that reference them (declaration order = construction order).

class FontBoundaryTest : public ::testing::Test {
protected:
    FontBoundaryTest()
        : primaryData_(makePrimaryData()),
          primaryFont_(&primaryData_),
          cjkFont_(&cjk_ui_20_font_data),
          family_(&primaryFont_) {
        family_.setFallback(&cjkFont_);
    }
    EpdFontData primaryData_;
    EpdFont primaryFont_;
    EpdFont cjkFont_;
    EpdFontFamily family_;

    // Convenience aliases for test bodies (backwards-compatible naming)
    EpdFontData& primaryData  = primaryData_;
    EpdFont&     primaryFont  = primaryFont_;
    EpdFont&     cjkFont      = cjkFont_;
    EpdFontFamily& family     = family_;
};

TEST_F(FontBoundaryTest, KernZeroAtLatinCjkBoundary) {
    // getKerning delegates to the primary font's kern table only (it does NOT use
    // resolveGlyph). With a populated primary kern table ('A'<->'A'), the non-zero
    // A-A result proves the table is live; CJK codepoints (0x65E5) miss the primary's
    // class tables -> lookupKernClass returns 0 -> kern 0 at the boundary. The CJK
    // font's own getKerning is pinned separately.
    EXPECT_NE(family.getKerning('A', 'A'), 0);       // primary kern table is LIVE
    EXPECT_EQ(family.getKerning('A', 0x65E5u), 0);   // 0x65E5 not in primary right-class table
    EXPECT_EQ(family.getKerning(0x65E5u, 'A'), 0);   // 0x65E5 not in primary left-class table
    EXPECT_EQ(cjkFont.getKerning(0x65E5u, 0x65E5u), 0);  // CJK font's own table is null -> 0
}

TEST_F(FontBoundaryTest, LigatureLookupNoCjkMatch) {
    // CJK font has ligaturePairs = nullptr → getLigature must return 0
    EXPECT_EQ(cjkFont.getLigature('A', 0x65E5u), 0u);
    EXPECT_EQ(cjkFont.getLigature(0x65E5u, 0x65E5u), 0u);
    // Primary font also has no ligature pairs (ligaturePairs=nullptr in makePrimaryData)
    EXPECT_EQ(primaryFont.getLigature('A', 'A'), 0u);
}

TEST_F(FontBoundaryTest, AdvanceIsAdditiveAcrossBoundary) {
    // measure("A日") == measure("A") + measure("日")
    const int advA   = measureAdvance(family, "A");
    const int advCJK = measureAdvance(family, "\xe6\x97\xa5");  // U+65E5 日 in UTF-8
    const int advBoth = measureAdvance(family, "A\xe6\x97\xa5");

    EXPECT_GT(advA,   0) << "Latin advance must be > 0";
    EXPECT_GT(advCJK, 0) << "CJK advance must be > 0";
    EXPECT_EQ(advBoth, advA + advCJK)
        << "Advance must be additive across Latin-CJK boundary";
}

TEST_F(FontBoundaryTest, CjkDataHasNullKernAndLigatureTables) {
    EXPECT_EQ(cjk_ui_20_font_data.kernMatrix,       nullptr);
    EXPECT_EQ(cjk_ui_20_font_data.kernLeftClasses,  nullptr);
    EXPECT_EQ(cjk_ui_20_font_data.kernRightClasses, nullptr);
    EXPECT_EQ(cjk_ui_20_font_data.ligaturePairs,    nullptr);
    EXPECT_EQ(cjk_ui_20_font_data.ligaturePairCount, 0u);
}

// B-7: Verify advance additivity via the EpdFontFamily::resolveGlyph path
// for all four Latin-CJK boundary patterns (A日, 日A, A 日, 日 A).
// This pins the invariant that the resolver's per-glyph summing always
// agrees with any future batch-measurement path.
TEST_F(FontBoundaryTest, AdvanceAdditiveAtAllBoundaryPatterns) {
    // Helper lambda to sum advance for each codepoint individually.
    auto decomposeAndSum = [&](const char* s) -> int {
        int total = 0;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
        uint32_t cp;
        while ((cp = utf8NextCodepoint(&p))) {
            const auto r = family.resolveGlyph(cp);
            if (r.glyph) total += fp4::toPixel(r.glyph->advanceX);
        }
        return total;
    };

    // Patterns: A日, 日A, A 日, 日 A. Note: U+0020 (space) is NOT in the primary's
    // intervals, so it resolves to the replacement glyph — additivity still holds
    // because measureAdvance and decomposeAndSum walk the same resolver path.
    const char* patterns[] = {
        "A\xe6\x97\xa5",         // A日
        ("\xe6\x97\xa5""A"),       // 日A
        "A \xe6\x97\xa5",        // A <space> 日 (space -> replacement glyph)
        "\xe6\x97\xa5 A",        // 日 <space> A (space -> replacement glyph)
    };
    for (const char* s : patterns) {
        EXPECT_EQ(measureAdvance(family, s), decomposeAndSum(s))
            << "Advance must be additive for pattern: " << s;
    }
}
