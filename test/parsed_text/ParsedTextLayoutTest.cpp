#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "lib/Epub/Epub/ParsedText.h"
#include "lib/Epub/Epub/blocks/TextBlock.h"
#include "lib/Epub/Epub/blocks/BlockStyle.h"
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
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false, /*focusReadingEnabled=*/false, leftAligned());
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
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false, /*focusReadingEnabled=*/false, leftAligned());
  text.addWord("x", EpdFontFamily::REGULAR);  // join=Space (default)
  text.addWord("200", EpdFontFamily::REGULAR);  // join=Space — breakable before
  text.addWord(" ", EpdFontFamily::REGULAR, /*underline=*/false, /*attachToPrevious=*/true);  // NBSP — Glue
  text.addWord("km", EpdFontFamily::REGULAR, /*underline=*/false, /*attachToPrevious=*/true);  // Glue
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
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/true, /*focusReadingEnabled=*/false, leftAligned());
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
  EXPECT_EQ(joined, "supercalifragilistic")
      << "hyphenation must not drop or duplicate characters across lines";
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
  ParsedText text(/*extraParagraphSpacing=*/false, /*hyphenationEnabled=*/false, /*focusReadingEnabled=*/false, leftAligned());
  text.addWord("abc", EpdFontFamily::REGULAR);                // 3 cells, LTR
  text.addWord("\xD7\x90\xD7\x91", EpdFontFamily::REGULAR);   // אב — 2 cells, RTL (Hebrew aleph-bet)
  text.addWord("xyz", EpdFontFamily::REGULAR);                // 3 cells, LTR
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
