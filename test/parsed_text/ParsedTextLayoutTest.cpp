#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "lib/Epub/Epub/ParsedText.h"
#include "lib/Epub/Epub/blocks/BlockStyle.h"
#include "lib/Epub/Epub/blocks/TextBlock.h"
#include "support/FakeTextMetrics.h"

namespace {

constexpr int kFontId = 0;
constexpr int kCell = 10;  // px per character cell

// Each produced line's visible characters (whitespace is NOT stored as words,
// so this is glued — we assert break positions, not spacing).
std::vector<std::string> linesOf(ParsedText& text, int viewportWidthPx) {
  FakeTextMetrics metrics(kCell);
  std::vector<std::string> lines;
  text.layoutAndExtractLines(metrics, kFontId, static_cast<uint16_t>(viewportWidthPx),
                             [&](std::shared_ptr<TextBlock> block) {
                               std::string joined;
                               for (const auto& w : block->getWords()) joined += w;
                               lines.push_back(joined);
                             });
  return lines;
}

// Round-8 M3: layout helper that returns the TextBlock per line, not just the
// joined string. Needed by BiDi xpos assertions and any test that wants to
// inspect getWordXpos() / getWordStyles() / getWords() together. linesOf() is
// still preferred for break-position-only tests because its string return type
// keeps the assertions concise; layoutBlocksOf() is the escape hatch when a
// test needs the full TextBlock surface.
std::vector<std::shared_ptr<TextBlock>> layoutBlocksOf(ParsedText& text, int viewportWidthPx) {
  FakeTextMetrics metrics(kCell);
  std::vector<std::shared_ptr<TextBlock>> blocks;
  text.layoutAndExtractLines(metrics, kFontId, static_cast<uint16_t>(viewportWidthPx),
                             [&](std::shared_ptr<TextBlock> block) { blocks.push_back(block); });
  return blocks;
}

BlockStyle leftAligned() {
  // BlockStyle default is Justify with no textIndent override, so resolveFirstLineIndent
  // can fall back to "3 × space width" when extraParagraphSpacing=false. Pin both axes
  // explicitly so layout tests measure only the break logic, not the first-line indent.
  BlockStyle s;
  s.alignment = CssTextAlign::Left;
  s.textAlignDefined = true;
  s.textIndent = 0;
  s.textIndentDefined = true;
  return s;
}

}  // namespace

// English wraps at word boundaries. Whitespace is dropped, so the joined text
// of each line is space-less; we pin WHERE the breaks fall.
TEST(ParsedTextLayout, EnglishWrapsAtWordBoundaries) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false, /*focusReadingEnabled=*/false,
                  leftAligned());
  for (const char* w : {"the", "quick", "brown", "fox"}) text.addWord(w, EpdFontFamily::REGULAR);
  // viewport 9 cells: "the"(3)+space+"quick"(5) = 9 → fits (computeLineBreaks at
  // ParsedText.cpp:407 uses strict `>` against viewport, so content==viewport fits).
  // Next line: "brown"(5)+space+"fox"(3) = 9 → also fits. Total: 2 lines, not 3.
  auto lines = linesOf(text, 9 * kCell);
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "thequick");  // space-less by design (gap stored as xpos)
  EXPECT_EQ(lines[1], "brownfox");
}

// No-break space IS an explicit " " word, so it appears in getWords().
//
// Round-9 C2: viewport sizing rationale. The earlier "viewport=5 cells / content=6 cells"
// form was wrong — at the DP layer (ParsedText.cpp:407, strict `>`), 6 cells of content
// against a 5-cell viewport OVERFLOWS, and Glue forbids any break inside the run. With
// no valid break point, the fallback in `computeLineBreaks` (~L439-447) sets `ans[i] = i`
// (single-word line), and the result is `"200"` on line 0 alone — NOT `"200 km"` together.
// The test would have asserted lines.size()==1 against a fallback that produces a different
// single-line shape than intended ("200" only), confirming nothing about Glue policy.
//
// Correct setup: viewport=6 cells exactly accommodates "200" (3) + " " (1) + "km" (2) =
// 6 cells; under strict `>`, content == viewport fits on one line. To prove Glue does
// real work, juxtapose against a token that could otherwise produce a break:
//   tokens "x", "200", NBSP, "km"  at viewport=6 cells
//   "x"(1) fits; can break after "x". "200"(3) + " "(1, Glue) + "km"(2) = 6 cells fits.
// Expected output: 2 lines — "x" and "200 km". Without Glue between " " and "km",
// the layout would prefer "x 200" on line 0 (1+1+3 = 5 ≤ 6) and " km" on line 1,
// which is a wrong shape for a no-break-space group.
TEST(ParsedTextLayout, NoBreakSpaceGlueKeepsGroupAtomic) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false, /*focusReadingEnabled=*/false,
                  leftAligned());
  text.addWord("x", EpdFontFamily::REGULAR);    // join=Space (default)
  text.addWord("200", EpdFontFamily::REGULAR);  // join=Space — breakable before
  text.addWord(" ", EpdFontFamily::REGULAR, /*underline=*/false, WordJoin::Glue);   // NBSP — Glue
  text.addWord("km", EpdFontFamily::REGULAR, /*underline=*/false, WordJoin::Glue);  // Glue
  auto lines = linesOf(text, 6 * kCell);
  // Glue forbids breaking between "200", " ", and "km" — the trio must move as one
  // unit. The optimal layout puts "x" on line 0 and "200 km" on line 1.
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "x");
  EXPECT_EQ(lines[1], "200 km");  // the explicit " " word is present in the join
}

// Hyphenation: a word longer than the line splits via fallback breaks.
TEST(ParsedTextLayout, LongWordHyphenatesAcrossLines) {
  // Note: hyphenation is the 2nd arg (hyphenationEnabled=true). allowFallback is not a ParsedText
  // constructor argument — it is a layout-time parameter. This test enables hyphenation.
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/true, /*focusReadingEnabled=*/false,
                  leftAligned());
  text.addWord("supercalifragilistic", EpdFontFamily::REGULAR);  // 20 chars
  auto lines = linesOf(text, 8 * kCell);

  // Round-9 M8: pin the WHOLE vector, not just lines[0] / lines[1]. A partial
  // assertion lets a regression that drops, duplicates, or reorders any line
  // from index 2 onwards pass silently. The hyphenation algorithm is
  // deterministic for a given font width + viewport, so the full `lines`
  // vector IS the contract.
  //
  // These are characterized (pinned) current-behaviour outputs of the Liang
  // hyphenation algorithm for this input + 8-cell viewport.
  const std::vector<std::string> expectedLines = {
      "superca-",
      "lifragi-",
      "listic",
  };
  EXPECT_EQ(lines, expectedLines);

  // Sanity: concatenating all lines must recover the input word (modulo
  // optional hyphen characters added by Liang hyphenation). Strip '-' before
  // comparing so the integrity check doesn't depend on the exact break shape.
  std::string joined;
  for (const auto& l : lines) joined += l;
  joined.erase(std::remove(joined.begin(), joined.end(), '-'), joined.end());
  EXPECT_EQ(joined, "supercalifragilistic") << "hyphenation must not drop or duplicate characters across lines";
}

// BiDi: an RTL run reorders visually. Pin reorder itself by asserting the visual
// word order, not only xpos.
//
// Round-9 M1: an xpos-only assertion does NOT directly characterize whether reorder
// happened — three different orderings (`abc אב xyz`, `אב abc xyz`, `xyz אב abc`)
// can land on the same xpos column under degenerate widths. The non-vacuous pin is
// `blocks[0]->getWords()` returning the visual order. We use asymmetric widths
// (`abc`=3 cells, `אב`=2 cells, `xyz`=3 cells, total=8 ≤ 20 → 1 line) so that any
// reorder regression also moves the xpos boundaries (auxiliary assert below).
TEST(ParsedTextLayout, BidiRtlRunLaysOut) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false, /*focusReadingEnabled=*/false,
                  leftAligned());
  text.addWord("abc", EpdFontFamily::REGULAR);               // 3 cells, LTR
  text.addWord("\xD7\x90\xD7\x91", EpdFontFamily::REGULAR);  // אב — 2 cells, RTL (Hebrew aleph-bet)
  text.addWord("xyz", EpdFontFamily::REGULAR);               // 3 cells, LTR
  auto blocks = layoutBlocksOf(text, 20 * kCell);
  ASSERT_EQ(blocks.size(), 1u);
  ASSERT_EQ(blocks[0]->getWords().size(), 3u);

  // PRIMARY assertion: visual order is the contract. These are characterized
  // (pinned) current-behaviour outputs of the BiDi reorder for this input.
  const auto& words = blocks[0]->getWords();
  EXPECT_EQ(words[0], "xyz");
  EXPECT_EQ(words[1], "\xD7\x90\xD7\x91");  // אב — Hebrew aleph-bet, visually RTL
  EXPECT_EQ(words[2], "abc");

  // AUXILIARY assertion: xpos sequence (helpful for diagnosing reorder regressions
  // because reorder shifts the LTR runs' x-positions).
  const auto& xpos = blocks[0]->getWordXpos();
  ASSERT_EQ(xpos.size(), 3u);
  EXPECT_EQ(xpos[0], static_cast<int16_t>(0));
  EXPECT_EQ(xpos[1], static_cast<int16_t>(40));
  EXPECT_EQ(xpos[2], static_cast<int16_t>(70));
}

// ─────────────────────────────────────────────────────────────────────────
// Phase 2 (PR E): Japanese / CJK layout characterization.
// ─────────────────────────────────────────────────────────────────────────

// Count UTF-8 codepoints in a string (used to assert CJK line lengths).
static size_t countCodepoints(const std::string& s) {
  size_t count = 0;
  const auto* p = reinterpret_cast<const unsigned char*>(s.c_str());
  while (utf8NextCodepoint(&p) != 0) ++count;
  return count;
}

// Add a CJK run: each char its own breakable, no-space word (mirrors the parser).
static void addCjkRun(ParsedText& text, int count, const char* ch) {
  for (int i = 0; i < count; ++i) {
    text.addWord(ch, EpdFontFamily::REGULAR, /*underline=*/false,
                 i == 0 ? WordJoin::Space : WordJoin::CjkBreak);
  }
}

TEST(ParsedTextLayout, JapaneseWrapsAtViewportNotAtChunk) {
  ParsedText text(false, false, false, leftAligned());
  addCjkRun(text, 25, "\xE8\x87\xAA");  // 25 × U+81EA (自); viewport 10 → 10/10/5
  auto lines = linesOf(text, 10 * kCell);
  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(countCodepoints(lines[0]), 10u);
  EXPECT_EQ(countCodepoints(lines[1]), 10u);
  EXPECT_EQ(countCodepoints(lines[2]), 5u);  // only the LAST line is short
}

// "English 日本語" — the space after Latin must be preserved as Space before the
// first CJK char; only subsequent CJK chars use CjkBreak.
TEST(ParsedTextLayout, ParserSequenceEnglishSpaceCjkPreservesSpace) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false,
                  /*focusReadingEnabled=*/false, leftAligned());
  text.addWord("English", EpdFontFamily::REGULAR);  // join=Space (default)
  text.addWord("\xE6\x97\xA5", EpdFontFamily::REGULAR, false, WordJoin::Space);     // 日 — first CJK after space
  text.addWord("\xE6\x9C\xAC", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);  // 本
  text.addWord("\xE8\xAA\x9E", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);  // 語
  std::shared_ptr<TextBlock> firstLine;
  FakeTextMetrics metrics(kCell);
  text.layoutAndExtractLines(metrics, kFontId, static_cast<uint16_t>(100 * kCell),
                             [&](std::shared_ptr<TextBlock> b) { if (!firstLine) firstLine = b; });
  ASSERT_TRUE(firstLine);
  const auto& xpos = firstLine->getWordXpos();
  ASSERT_GE(xpos.size(), 2u);
  // "English" is 7 codepoints × kCell = 70px wide. The 日 must sit kCell to the right
  // of English's tail (Space adds 1 cell). Under CjkBreak the delta would be ≈ 0
  // (kerning only). Pin the Space-induced gap explicitly:
  EXPECT_EQ(xpos[1] - xpos[0], 7 * kCell + kCell)
      << "first CJK char must inherit Space gap from preceding Latin word, not CjkBreak";
}

// "日本語English" — Latin after a CJK run inherits CjkBreak from the last CJK emit,
// so it attaches without a space (intentional — Japanese typography norm).
TEST(ParsedTextLayout, ParserSequenceCjkLatinNoSpace) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false,
                  /*focusReadingEnabled=*/false, leftAligned());
  text.addWord("\xE6\x97\xA5", EpdFontFamily::REGULAR, false, WordJoin::Space);     // 日 (paragraph start)
  text.addWord("\xE6\x9C\xAC", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);  // 本
  text.addWord("\xE8\xAA\x9E", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);  // 語
  text.addWord("English", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);       // Latin after CJK
  std::shared_ptr<TextBlock> firstLine;
  FakeTextMetrics metrics(kCell);
  text.layoutAndExtractLines(metrics, kFontId, static_cast<uint16_t>(100 * kCell),
                             [&](std::shared_ptr<TextBlock> b) { if (!firstLine) firstLine = b; });
  ASSERT_TRUE(firstLine);
  const auto& xpos = firstLine->getWordXpos();
  ASSERT_GE(xpos.size(), 4u);
  // Each CJK char is kCell wide. "English" must sit flush against 語's tail (CjkBreak
  // adds no space): xpos[3] - xpos[2] should equal kCell (just 語's width), not 2*kCell.
  EXPECT_EQ(xpos[3] - xpos[2], kCell)
      << "Latin word after CJK run must inherit CjkBreak (no space gap)";
}

// Layout-layer characterization for a CJK-Latin-CJK sequence (does NOT exercise the
// parser path — the parser's caller-append duty is verified by PR F's assembler unit
// tests + device QA). Guards the focus / xpos / cluster-token side of the contract.
TEST(ParsedTextLayout, JapaneseRunWithSingleLatinCharNotDropped) {
  ParsedText text(false, false, false, leftAligned());
  text.addWord("\xE6\x97\xA5", EpdFontFamily::REGULAR, false, WordJoin::Space);     // 日
  text.addWord("a", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);             // Latin 1 char
  text.addWord("\xE6\x9C\xAC", EpdFontFamily::REGULAR, false, WordJoin::CjkBreak);  // 本
  auto lines = linesOf(text, 100 * kCell);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_NE(lines[0].find("a"), std::string::npos);  // 'a' must survive
  EXPECT_EQ(countCodepoints(lines[0]), 3u);
}

// NBSP glue around a CJK token: the CJK char inherits Glue from the NBSP, not
// CjkBreak, so the layout must not break between them. Leading breakable "x" forces a
// valid break BETWEEN "x" and "200", not inside the NBSP group.
TEST(ParsedTextLayout, ParserSequenceNbspGluesCjk) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false,
                  /*focusReadingEnabled=*/false, leftAligned());
  text.addWord("x", EpdFontFamily::REGULAR);                                         // join=Space
  text.addWord("200", EpdFontFamily::REGULAR);                                       // join=Space
  text.addWord(" ", EpdFontFamily::REGULAR, false, WordJoin::Glue);                  // NBSP word
  text.addWord("\xE8\xAA\x9E", EpdFontFamily::REGULAR, false, WordJoin::Glue);       // 語 inherits Glue
  auto lines = linesOf(text, 6 * kCell);
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "x");
  EXPECT_EQ(lines[1], "200 語");  // NBSP word " " is preserved as an explicit token in the join
}

// Focus reading must NOT bold CJK 1-char tokens (Task 2.1 focus-bypass). The only
// observable difference is whether the CJK tokens carry the BOLD style flag, so this
// asserts on getWordStyles(), not on token count.
TEST(ParsedTextLayout, FocusReadingDoesNotBoldCjkTokens) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false,
                  /*focusReadingEnabled=*/true, leftAligned());
  addCjkRun(text, 25, "\xE8\x87\xAA");  // 25 × 自 with CjkBreak joins (first is Space)
  std::vector<EpdFontFamily::Style> styles;
  text.layoutAndExtractLines(FakeTextMetrics(kCell), kFontId, 100 * kCell,
                             [&](std::shared_ptr<TextBlock> block) {
                               for (const auto& s : block->getWordStyles()) styles.push_back(s);
                             });
  ASSERT_EQ(styles.size(), 25u);  // sanity: every CJK char emitted exactly one style entry
  for (size_t i = 0; i < styles.size(); ++i) {
    EXPECT_EQ(static_cast<int>(styles[i]) & static_cast<int>(EpdFontFamily::BOLD), 0)
        << "CJK token " << i << " unexpectedly has BOLD set — Step 2.5 bypass did not fire";
  }
}

// Multi-codepoint NFD cluster bypass: the parser emits 2-codepoint tokens like
// か (U+304B) + U+3099. A single-codepoint gate would split base from extender at
// render time. Assert the grapheme-cluster gate accepts them too.
TEST(ParsedTextLayout, FocusReadingDoesNotBoldCjkNfdClusters) {
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false,
                  /*focusReadingEnabled=*/true, leftAligned());
  // Build 25 NFD clusters: か + U+3099 = "\xE3\x81\x8B\xE3\x82\x99" (6 bytes, 2 codepoints).
  const std::string kCluster = "\xE3\x81\x8B\xE3\x82\x99";
  for (int i = 0; i < 25; ++i) {
    text.addWord(kCluster, EpdFontFamily::REGULAR, /*underline=*/false,
                 i == 0 ? WordJoin::Space : WordJoin::CjkBreak);
  }
  std::vector<EpdFontFamily::Style> styles;
  text.layoutAndExtractLines(FakeTextMetrics(kCell), kFontId, 100 * kCell,
                             [&](std::shared_ptr<TextBlock> block) {
                               for (const auto& s : block->getWordStyles()) styles.push_back(s);
                             });
  ASSERT_EQ(styles.size(), 25u);
  for (size_t i = 0; i < styles.size(); ++i) {
    EXPECT_EQ(static_cast<int>(styles[i]) & static_cast<int>(EpdFontFamily::BOLD), 0)
        << "NFD cluster " << i << " unexpectedly has BOLD set — isSingleCjkGraphemeCluster missed it";
  }
}

// Note: the empty-word focus test is deliberately omitted. ParsedText::addWord
// short-circuits with `if (word.empty()) return;` BEFORE the focus-bypass helper,
// so empty words never reach isSingleCjkGraphemeCluster — such a test would be vacuous.
