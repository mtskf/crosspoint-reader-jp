// lib/Epub/Epub/parsers/Utf8ClusterAssembler.h
#pragma once

#include <cstdint>

#include "../WordJoin.h"     // for WordJoin (enum-only, no layout deps)
#include <EpdFontFamily.h>   // for EpdFontFamily::Style (used by State + Flushable + callerFontStyle)
#include <Utf8.h>            // for utf8NextCodepoint + predicates

class Utf8ClusterAssembler {
 public:
  // Classifies the non-CJK codepoint that the caller must dispatch on after an
  // EmittedAndNonCjk / NonCjkOnly return. The four kinds map 1:1 to the four branches
  // in ChapterHtmlSlimParser::dispatchNonCjk (Latin buffer append / whitespace flush /
  // NBSP synthesized token / FEFF discard).
  enum class NonCjkKind : uint8_t {
    Latin,        // ordinary printable codepoint — caller appends raw UTF-8 bytes to Latin buffer
    Whitespace,   // U+0020 / U+0009 / U+000A / U+000D — flush Latin, set nextJoin = Space
    Nbsp,         // U+00A0 / U+202F — flush, emit synthesized " " token with Glue, then nextJoin = Glue
    Feff,         // U+FEFF (BOM) — discard
  };

  enum class ConsumeResult : uint8_t {
    NeedMore,           // current codepoint incomplete (carry-over staged); i unchanged
    StagedOnly,         // codepoint consumed, staged into pendingCjkBase, no flushable emitted; i advanced
    EmittedFlushable,   // staged base flushed (caller emits it as a CJK token);
                        // current codepoint also staged/consumed; i advanced
    NonCjkOnly,         // no pending base existed (outFlushable INVALID — do not read).
                        // outNonCjkKind + outNonCjkCp + outNonCjkLen describe the non-CJK cp; i advanced.
    EmittedAndNonCjk,   // a pending base existed and was flushed: outFlushable filled.
                        // outNonCjkKind + outNonCjkCp + outNonCjkLen describe the non-CJK cp; i advanced.
  };

  // Cross-callback state — owned by the parser, threaded into each call.
  // fontStyleAtStage / underlineAtStage are snapshotted at STAGE time (not flush time): a CJK
  // base may be staged inside <u> and flushed after </u> closes; the flush must use the style
  // in effect when the base was STAGED.
  struct State {
    uint8_t pendingUtf8[4] = {};       // leading bytes of a truncated codepoint
    uint8_t pendingUtf8Len = 0;        // 0..3
    char pendingCjkBase[16] = {};      // staged base + already-absorbed extenders
    uint8_t pendingCjkBaseLen = 0;
    WordJoin pendingCjkBaseJoin = WordJoin::Space;
    EpdFontFamily::Style fontStyleAtStage = EpdFontFamily::REGULAR;
    bool underlineAtStage = false;
  };

  // Payload emitted when a previously-staged CJK base must be flushed.
  struct Flushable {
    char bytes[16];
    uint8_t len;
    WordJoin join;
    EpdFontFamily::Style fontStyle;   // snapshot at stage time
    bool underline;                    // snapshot at stage time
  };

  // Consume up to one complete codepoint from s[i..len]. Returns the discriminant; out-params
  // populated per the state-transition table. callerFontStyle / callerUnderline are the resolved
  // style at THIS call; when a new base is staged they are snapshotted into State.
  static ConsumeResult tryConsumeCodepoint(
      const char* s, int len, int& i,
      State& state,
      WordJoin& nextJoin,
      EpdFontFamily::Style callerFontStyle,
      bool callerUnderline,
      Flushable& outFlushable,      // valid iff result in {EmittedFlushable, EmittedAndNonCjk}
      NonCjkKind& outNonCjkKind,    // valid iff result in {NonCjkOnly, EmittedAndNonCjk}
      uint32_t& outNonCjkCp,        // valid iff result in {NonCjkOnly, EmittedAndNonCjk}
      uint8_t& outNonCjkLen);       // total UTF-8 byte length of the non-CJK codepoint (1..4)

  // Force-flush any staged base. Returns true with outFlushable filled if a base was pending.
  static bool flushPendingBase(State& state, Flushable& outFlushable);
};
