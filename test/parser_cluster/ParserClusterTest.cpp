// test/parser_cluster/ParserClusterTest.cpp
//
// Behavioural contract for Utf8ClusterAssembler — the host-safe state machine that
// performs UTF-8 decode + grapheme cluster assembly + WordJoin policy. The parser
// (Task 2.3c) consumes it; these 12 cases pin the contract that parser relies on.
//
// Helper `feed()` drives tryConsumeCodepoint over a byte buffer and collects every
// emitted Flushable, then drains any staged base with flushPendingBase.

#include <Utf8.h>
#include <Utf8ClusterAssembler.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

using ConsumeResult = Utf8ClusterAssembler::ConsumeResult;
using NonCjkKind = Utf8ClusterAssembler::NonCjkKind;
using Flushable = Utf8ClusterAssembler::Flushable;
using State = Utf8ClusterAssembler::State;

// One observed call result, for assertions on call-by-call behaviour.
struct Step {
  ConsumeResult result;
  int iBefore;
  int iAfter;
  Flushable flushable;  // valid iff result in {EmittedFlushable, EmittedAndNonCjk}
  NonCjkKind nonCjkKind;
  uint32_t nonCjkCp;
  uint8_t nonCjkLen;
};

// Drive tryConsumeCodepoint across a whole buffer. callerFontStyle/callerUnderline are
// held constant for the whole feed (per-call snapshots are exercised directly in the
// style-snapshot tests). Stops looping when i no longer advances on a NeedMore so the
// caller can inspect the carry-over.
std::vector<Step> feed(const std::vector<uint8_t>& bytes, State& state, WordJoin& nextJoin,
                       EpdFontFamily::Style callerFontStyle = EpdFontFamily::REGULAR, bool callerUnderline = false) {
  std::vector<Step> steps;
  const char* s = reinterpret_cast<const char*>(bytes.data());
  const int len = static_cast<int>(bytes.size());
  int i = 0;
  while (i < len) {
    Step step;
    step.iBefore = i;
    step.flushable = Flushable{};
    step.nonCjkKind = NonCjkKind::Latin;
    step.nonCjkCp = 0;
    step.nonCjkLen = 0;
    step.result =
        Utf8ClusterAssembler::tryConsumeCodepoint(s, len, i, state, nextJoin, callerFontStyle, callerUnderline,
                                                  step.flushable, step.nonCjkKind, step.nonCjkCp, step.nonCjkLen);
    step.iAfter = i;
    steps.push_back(step);
    if (step.result == ConsumeResult::NeedMore) break;  // carry-over staged; no more progress this buffer
  }
  return steps;
}

uint32_t decodeOnly(const char* bytes, uint8_t len) {
  char scratch[5] = {};
  memcpy(scratch, bytes, len);
  const unsigned char* p = reinterpret_cast<const unsigned char*>(scratch);
  return utf8NextCodepoint(&p);
}

// 1. NUL termination boundary: a 3-byte CJK cp where `len` excludes any trailing NUL.
//    Classification fires only after the full cp is present; no read past s[len-1].
TEST(ParserCluster, NulTerminationBoundary) {
  // 日 U+65E5 = E6 97 A5, buffer holds exactly 3 bytes, no trailing NUL.
  const std::vector<uint8_t> buf = {0xE6, 0x97, 0xA5};
  State state;
  WordJoin nextJoin = WordJoin::Space;
  auto steps = feed(buf, state, nextJoin);
  ASSERT_EQ(steps.size(), 1u);
  EXPECT_EQ(steps[0].result, ConsumeResult::StagedOnly);
  EXPECT_EQ(steps[0].iAfter, 3);  // consumed all 3 bytes, none past the boundary
  // The staged base is the full cp.
  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 3);
  EXPECT_EQ(decodeOnly(drained.bytes, drained.len), 0x65E5u);
}

// 2. Carry-over across calls: split U+65E5 as {E6,97} then {A5}.
TEST(ParserCluster, CarryOverAcrossCalls) {
  State state;
  WordJoin nextJoin = WordJoin::Space;

  // Call 1: first 2 bytes only -> NeedMore, i unchanged, no flushable.
  const std::vector<uint8_t> part1 = {0xE6, 0x97};
  const char* s1 = reinterpret_cast<const char*>(part1.data());
  int i1 = 0;
  Flushable f1{};
  NonCjkKind k1 = NonCjkKind::Latin;
  uint32_t cp1 = 0;
  uint8_t l1 = 0;
  auto r1 = Utf8ClusterAssembler::tryConsumeCodepoint(s1, 2, i1, state, nextJoin, EpdFontFamily::REGULAR, false, f1, k1,
                                                      cp1, l1);
  EXPECT_EQ(r1, ConsumeResult::NeedMore);
  EXPECT_EQ(i1, 0);  // i unchanged
  EXPECT_EQ(state.pendingUtf8Len, 2);

  // Call 2: final byte -> stages the cluster.
  const std::vector<uint8_t> part2 = {0xA5};
  const char* s2 = reinterpret_cast<const char*>(part2.data());
  int i2 = 0;
  Flushable f2{};
  NonCjkKind k2 = NonCjkKind::Latin;
  uint32_t cp2 = 0;
  uint8_t l2 = 0;
  auto r2 = Utf8ClusterAssembler::tryConsumeCodepoint(s2, 1, i2, state, nextJoin, EpdFontFamily::REGULAR, false, f2, k2,
                                                      cp2, l2);
  EXPECT_EQ(r2, ConsumeResult::StagedOnly);
  EXPECT_EQ(i2, 1);

  // Drain emits one 3-byte cluster.
  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 3);
  EXPECT_EQ(decodeOnly(drained.bytes, drained.len), 0x65E5u);
}

// 3. Single-callback NFD cluster: か (U+304B) + U+3099 in one call.
TEST(ParserCluster, SingleCallbackNfdCluster) {
  // か U+304B = E3 81 8B, U+3099 = E3 82 99
  const std::vector<uint8_t> buf = {0xE3, 0x81, 0x8B, 0xE3, 0x82, 0x99};
  State state;
  WordJoin nextJoin = WordJoin::Space;
  auto steps = feed(buf, state, nextJoin);
  ASSERT_EQ(steps.size(), 2u);
  EXPECT_EQ(steps[0].result, ConsumeResult::StagedOnly);  // base staged
  EXPECT_EQ(steps[1].result, ConsumeResult::StagedOnly);  // extender absorbed, no flush
  EXPECT_EQ(state.pendingCjkBaseLen, 6);                  // 3 + 3 bytes glued

  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 6);  // one 6-byte cluster spanning both codepoints
}

// 4. Cross-callback NFD cluster: か in call 1 + U+3099 in call 2.
TEST(ParserCluster, CrossCallbackNfdCluster) {
  State state;
  WordJoin nextJoin = WordJoin::Space;

  const std::vector<uint8_t> call1 = {0xE3, 0x81, 0x8B};  // か
  auto s1 = feed(call1, state, nextJoin);
  ASSERT_EQ(s1.size(), 1u);
  EXPECT_EQ(s1[0].result, ConsumeResult::StagedOnly);
  EXPECT_EQ(state.pendingCjkBaseLen, 3);  // base survives the boundary

  const std::vector<uint8_t> call2 = {0xE3, 0x82, 0x99};  // U+3099
  auto s2 = feed(call2, state, nextJoin);
  ASSERT_EQ(s2.size(), 1u);
  EXPECT_EQ(s2[0].result, ConsumeResult::StagedOnly);  // extender glued onto carried base
  EXPECT_EQ(state.pendingCjkBaseLen, 6);

  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 6);  // one 6-byte cluster spanning both callbacks
}

// 5. Variation Selector VS-1: 漢 (U+6F22) + U+FE00.
TEST(ParserCluster, VariationSelectorVs1) {
  // 漢 = E6 BC A2, U+FE00 = EF B8 80
  const std::vector<uint8_t> buf = {0xE6, 0xBC, 0xA2, 0xEF, 0xB8, 0x80};
  State state;
  WordJoin nextJoin = WordJoin::Space;
  feed(buf, state, nextJoin);

  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 6);  // one cluster of exactly 6 bytes
}

// 6. Variation Selector Supplement: 漢 (3 bytes) + U+E0100 (4 bytes).
TEST(ParserCluster, VariationSelectorSupplement) {
  // 漢 = E6 BC A2, U+E0100 = F3 A0 84 80
  const std::vector<uint8_t> buf = {0xE6, 0xBC, 0xA2, 0xF3, 0xA0, 0x84, 0x80};
  State state;
  WordJoin nextJoin = WordJoin::Space;
  feed(buf, state, nextJoin);

  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 7);  // 3 + 4 bytes
}

// 7. Halfwidth voicing: ｶ (U+FF76) + ﾞ (U+FF9E).
TEST(ParserCluster, HalfwidthVoicing) {
  // ｶ U+FF76 = EF BD B6, ﾞ U+FF9E = EF BE 9E
  const std::vector<uint8_t> buf = {0xEF, 0xBD, 0xB6, 0xEF, 0xBE, 0x9E};
  State state;
  WordJoin nextJoin = WordJoin::Space;
  feed(buf, state, nextJoin);

  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 6);  // one 6-byte cluster
}

// 8. Ideographic tone mark: CJK base + U+302A. Pins extender-before-breakable ordering:
//    U+302A is BOTH a grapheme extender and CJK-breakable. If breakable were checked first
//    this would be two clusters and this test would go red.
TEST(ParserCluster, IdeographicToneMarkGluesNotBreaks) {
  // 日 = E6 97 A5, U+302A = E3 80 AA
  const std::vector<uint8_t> buf = {0xE6, 0x97, 0xA5, 0xE3, 0x80, 0xAA};
  State state;
  WordJoin nextJoin = WordJoin::Space;
  auto steps = feed(buf, state, nextJoin);
  ASSERT_EQ(steps.size(), 2u);
  EXPECT_EQ(steps[0].result, ConsumeResult::StagedOnly);  // base
  EXPECT_EQ(steps[1].result, ConsumeResult::StagedOnly);  // tone mark glued, NOT a new base
  EXPECT_EQ(state.pendingCjkBaseLen, 6);

  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(drained.len, 6);  // a single cluster, not two
}

// 9. Overflow guard with join reset: base + enough extenders that the next append would
//    exceed sizeof(pendingCjkBase) (16). The staged cluster force-flushes via
//    EmittedFlushable, the overflowing extender becomes the new staged base, and the new
//    base's join is reset to CjkBreak (NOT the inherited Glue/Space).
TEST(ParserCluster, OverflowGuardResetsJoin) {
  // Base 漢 (3 bytes) then VS-1 extenders (3 bytes each: EF B8 80).
  // 3 + 3 + 3 + 3 + 3 = 15 fits; the 5th extender (would make 18) overflows.
  std::vector<uint8_t> buf = {0xE6, 0xBC, 0xA2};                          // 漢, 3 bytes
  for (int n = 0; n < 5; ++n) buf.insert(buf.end(), {0xEF, 0xB8, 0x80});  // 5 × U+FE00

  State state;
  WordJoin nextJoin = WordJoin::Glue;  // inherited join is Glue, to prove reset to CjkBreak
  auto steps = feed(buf, state, nextJoin);

  // Find the force-flush step.
  bool sawEmittedFlushable = false;
  Flushable forced{};
  for (const auto& step : steps) {
    if (step.result == ConsumeResult::EmittedFlushable) {
      sawEmittedFlushable = true;
      forced = step.flushable;
    }
  }
  ASSERT_TRUE(sawEmittedFlushable);  // (a) staged cluster force-flushed
  EXPECT_EQ(forced.len, 15);         // 漢 + 4 extenders = full 15-byte cluster

  // (b) the overflowing extender is now the new staged base.
  EXPECT_EQ(state.pendingCjkBaseLen, 3);  // the 5th U+FE00 alone
  // (c) join reset to CjkBreak, NOT the inherited Glue.
  EXPECT_EQ(state.pendingCjkBaseJoin, WordJoin::CjkBreak);
}

// 10. EmittedAndNonCjk caller-append contract: 日 (U+65E5) then 'a' (U+0061).
//     The caller MUST re-encode outNonCjkCp via utf8AppendCodepoint (NOT copy from s[]).
TEST(ParserCluster, EmittedAndNonCjkCallerAppendContract) {
  // 日 = E6 97 A5, 'a' = 61
  const std::vector<uint8_t> buf = {0xE6, 0x97, 0xA5, 0x61};
  State state;
  WordJoin nextJoin = WordJoin::Space;

  const char* s = reinterpret_cast<const char*>(buf.data());
  const int len = static_cast<int>(buf.size());
  int i = 0;

  // Call A: stage 日 with underline=true, REGULAR.
  Flushable fa{};
  NonCjkKind ka = NonCjkKind::Latin;
  uint32_t cpa = 0;
  uint8_t la = 0;
  auto ra = Utf8ClusterAssembler::tryConsumeCodepoint(s, len, i, state, nextJoin, EpdFontFamily::REGULAR,
                                                      /*callerUnderline=*/true, fa, ka, cpa, la);
  EXPECT_EQ(ra, ConsumeResult::StagedOnly);
  EXPECT_EQ(i, 3);

  // Call B: consume 'a' with underline=false -> force flush 日.
  Flushable fb{};
  NonCjkKind kb = NonCjkKind::Latin;
  uint32_t cpb = 0;
  uint8_t lb = 0;
  auto rb = Utf8ClusterAssembler::tryConsumeCodepoint(s, len, i, state, nextJoin, EpdFontFamily::REGULAR,
                                                      /*callerUnderline=*/false, fb, kb, cpb, lb);
  EXPECT_EQ(rb, ConsumeResult::EmittedAndNonCjk);  // (a)
  EXPECT_EQ(fb.len, 3);                            // (b)
  EXPECT_EQ(decodeOnly(fb.bytes, fb.len), 0x65E5u);
  EXPECT_EQ(kb, NonCjkKind::Latin);       // (c)
  EXPECT_EQ(cpb, 0x0061u);                // (d)
  EXPECT_EQ(lb, 1);                       // (e)
  EXPECT_EQ(i, 4);                        // (f) advanced past 'a'
  EXPECT_EQ(state.pendingCjkBaseLen, 0);  // (g) state cleared
  // (h)(i) stage-time snapshot survives, NOT call-B's false/REGULAR.
  EXPECT_TRUE(fb.underline);
  EXPECT_EQ(fb.fontStyle, EpdFontFamily::REGULAR);

  // The caller MUST re-encode outNonCjkCp via utf8AppendCodepoint (NOT copy s[i_prev..i-1]).
  std::string reencoded;
  utf8AppendCodepoint(cpb, reencoded);
  ASSERT_EQ(reencoded.size(), 1u);
  EXPECT_EQ(static_cast<uint8_t>(reencoded[0]), 0x61);

  // NonCjkOnly sub-case: ' ' alone with no preceding base.
  {
    const std::vector<uint8_t> sp = {0x20};
    State st2;
    WordJoin nj2 = WordJoin::Space;
    const char* ss = reinterpret_cast<const char*>(sp.data());
    int j = 0;
    Flushable fx{};
    NonCjkKind kx = NonCjkKind::Latin;
    uint32_t cpx = 0;
    uint8_t lx = 0;
    auto rx =
        Utf8ClusterAssembler::tryConsumeCodepoint(ss, 1, j, st2, nj2, EpdFontFamily::REGULAR, false, fx, kx, cpx, lx);
    EXPECT_EQ(rx, ConsumeResult::NonCjkOnly);
    EXPECT_EQ(kx, NonCjkKind::Whitespace);  // do NOT read fx (outFlushable INVALID)
    EXPECT_EQ(j, 1);
  }

  // Other NonCjkKind variants after a staged base: 日 + ' '/NBSP/FEFF.
  auto classifyAfterBase = [](const std::vector<uint8_t>& tail) -> NonCjkKind {
    std::vector<uint8_t> b = {0xE6, 0x97, 0xA5};  // 日
    b.insert(b.end(), tail.begin(), tail.end());
    State st;
    WordJoin nj = WordJoin::Space;
    const char* ss = reinterpret_cast<const char*>(b.data());
    const int ln = static_cast<int>(b.size());
    int j = 0;
    NonCjkKind seen = NonCjkKind::Latin;
    while (j < ln) {
      Flushable fl{};
      NonCjkKind kk = NonCjkKind::Latin;
      uint32_t cc = 0;
      uint8_t ll = 0;
      auto r =
          Utf8ClusterAssembler::tryConsumeCodepoint(ss, ln, j, st, nj, EpdFontFamily::REGULAR, false, fl, kk, cc, ll);
      if (r == ConsumeResult::EmittedAndNonCjk || r == ConsumeResult::NonCjkOnly) {
        seen = kk;
        break;
      }
    }
    return seen;
  };
  EXPECT_EQ(classifyAfterBase({0x20}), NonCjkKind::Whitespace);        // ' '
  EXPECT_EQ(classifyAfterBase({0xC2, 0xA0}), NonCjkKind::Nbsp);        // NBSP U+00A0
  EXPECT_EQ(classifyAfterBase({0xEF, 0xBB, 0xBF}), NonCjkKind::Feff);  // FEFF
}

// 11. Split-Latin codepoint across callbacks: é (U+00E9 = C3 A9) split {C3} then {A9}.
TEST(ParserCluster, SplitLatinCodepointAcrossCallbacks) {
  State state;
  WordJoin nextJoin = WordJoin::Space;

  // Call 1: {C3} only -> NeedMore, carry-over staged, i unchanged.
  const std::vector<uint8_t> part1 = {0xC3};
  const char* s1 = reinterpret_cast<const char*>(part1.data());
  int i1 = 0;
  Flushable f1{};
  NonCjkKind k1 = NonCjkKind::Latin;
  uint32_t cp1 = 0;
  uint8_t l1 = 0;
  auto r1 = Utf8ClusterAssembler::tryConsumeCodepoint(s1, 1, i1, state, nextJoin, EpdFontFamily::REGULAR, false, f1, k1,
                                                      cp1, l1);
  EXPECT_EQ(r1, ConsumeResult::NeedMore);
  EXPECT_EQ(i1, 0);
  EXPECT_EQ(state.pendingUtf8Len, 1);

  // Call 2: {A9} -> NonCjkOnly, Latin, cp 0x00E9, len 2.
  const std::vector<uint8_t> part2 = {0xA9};
  const char* s2 = reinterpret_cast<const char*>(part2.data());
  int i2 = 0;
  Flushable f2{};
  NonCjkKind k2 = NonCjkKind::Latin;
  uint32_t cp2 = 0;
  uint8_t l2 = 0;
  auto r2 = Utf8ClusterAssembler::tryConsumeCodepoint(s2, 1, i2, state, nextJoin, EpdFontFamily::REGULAR, false, f2, k2,
                                                      cp2, l2);
  EXPECT_EQ(r2, ConsumeResult::NonCjkOnly);
  EXPECT_EQ(k2, NonCjkKind::Latin);
  EXPECT_EQ(cp2, 0x00E9u);
  EXPECT_EQ(l2, 2);  // FULL byte length, even though i advanced by only 1 continuation byte
  EXPECT_EQ(i2, 1);

  // Lossless re-encode even though the two bytes never appeared together in one callback.
  std::string scratch;
  utf8AppendCodepoint(cp2, scratch);
  ASSERT_EQ(scratch.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(scratch[0]), 0xC3);
  EXPECT_EQ(static_cast<uint8_t>(scratch[1]), 0xA9);
}

// 12. Style snapshot at stage time, NOT flush time. Proxies <u>日</u>本.
TEST(ParserCluster, StyleSnapshotAtStageTimeNotFlushTime) {
  // 日 = E6 97 A5, 本 U+672C = E6 9C AC
  const std::vector<uint8_t> buf = {0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC};
  State state;
  WordJoin nextJoin = WordJoin::Space;

  const char* s = reinterpret_cast<const char*>(buf.data());
  const int len = static_cast<int>(buf.size());
  int i = 0;

  // Call A: 日 under REGULAR + underline=true.
  Flushable fa{};
  NonCjkKind ka = NonCjkKind::Latin;
  uint32_t cpa = 0;
  uint8_t la = 0;
  auto ra = Utf8ClusterAssembler::tryConsumeCodepoint(s, len, i, state, nextJoin, EpdFontFamily::REGULAR,
                                                      /*callerUnderline=*/true, fa, ka, cpa, la);
  EXPECT_EQ(ra, ConsumeResult::StagedOnly);
  EXPECT_EQ(state.pendingCjkBaseLen, 3);
  EXPECT_TRUE(state.underlineAtStage);
  EXPECT_EQ(state.fontStyleAtStage, EpdFontFamily::REGULAR);

  // Call B: 本 under REGULAR + underline=false -> flush 日 with STAGE-time snapshot.
  Flushable fb{};
  NonCjkKind kb = NonCjkKind::Latin;
  uint32_t cpb = 0;
  uint8_t lb = 0;
  auto rb = Utf8ClusterAssembler::tryConsumeCodepoint(s, len, i, state, nextJoin, EpdFontFamily::REGULAR,
                                                      /*callerUnderline=*/false, fb, kb, cpb, lb);
  EXPECT_EQ(rb, ConsumeResult::EmittedFlushable);
  EXPECT_EQ(fb.len, 3);
  EXPECT_EQ(decodeOnly(fb.bytes, fb.len), 0x65E5u);  // 日
  EXPECT_TRUE(fb.underline);                         // stage-time true, NOT call-B false
  EXPECT_EQ(fb.fontStyle, EpdFontFamily::REGULAR);
  EXPECT_EQ(state.pendingCjkBaseLen, 3);  // 本 now staged
  EXPECT_FALSE(state.underlineAtStage);   // 本 staged under underline=false

  // Drain: 本 flushes with underline=false.
  Flushable drained;
  ASSERT_TRUE(Utf8ClusterAssembler::flushPendingBase(state, drained));
  EXPECT_EQ(decodeOnly(drained.bytes, drained.len), 0x672Cu);  // 本
  EXPECT_FALSE(drained.underline);

  // Symmetric sub-test: 日<u>本</u> — 日 staged under underline=false, 本 under true.
  {
    State st;
    WordJoin nj = WordJoin::Space;
    int j = 0;

    Flushable g1{};
    NonCjkKind gk1 = NonCjkKind::Latin;
    uint32_t gc1 = 0;
    uint8_t gl1 = 0;
    auto gr1 = Utf8ClusterAssembler::tryConsumeCodepoint(s, len, j, st, nj, EpdFontFamily::REGULAR,
                                                         /*callerUnderline=*/false, g1, gk1, gc1, gl1);
    EXPECT_EQ(gr1, ConsumeResult::StagedOnly);  // 日 staged under underline=false

    Flushable g2{};
    NonCjkKind gk2 = NonCjkKind::Latin;
    uint32_t gc2 = 0;
    uint8_t gl2 = 0;
    auto gr2 = Utf8ClusterAssembler::tryConsumeCodepoint(s, len, j, st, nj, EpdFontFamily::REGULAR,
                                                         /*callerUnderline=*/true, g2, gk2, gc2, gl2);
    EXPECT_EQ(gr2, ConsumeResult::EmittedFlushable);
    EXPECT_FALSE(g2.underline);        // 日's stage-time underline=false
    EXPECT_TRUE(st.underlineAtStage);  // 本 now staged under underline=true
  }
}

}  // namespace
