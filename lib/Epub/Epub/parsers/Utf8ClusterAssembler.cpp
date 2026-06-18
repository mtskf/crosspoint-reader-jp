// lib/Epub/Epub/parsers/Utf8ClusterAssembler.cpp
//
// Host-safe UTF-8 decode + grapheme cluster assembly + WordJoin policy. See the header
// for the public contract and the task brief for the state-transition table. No heap, no
// Arduino/HAL deps — depends only on Utf8.h + WordJoin.h + EpdFontFamily.h.

#include "Utf8ClusterAssembler.h"

#include <cstddef>
#include <cstring>

namespace {

// Expected total byte length of a UTF-8 sequence from its lead byte. Mirrors Utf8.cpp's
// utf8CodepointLen (kept local so this TU stays free of internal-linkage coupling).
uint8_t leadByteLen(uint8_t c) {
  if (c < 0x80) return 1;          // 0xxxxxxx
  if ((c >> 5) == 0x6) return 2;   // 110xxxxx
  if ((c >> 4) == 0xE) return 3;   // 1110xxxx
  if ((c >> 3) == 0x1E) return 4;  // 11110xxx
  return 1;                        // invalid lead — treat as a 1-byte (replacement) cp
}

// Classify a fully-decoded non-CJK codepoint into the caller-dispatch kind.
Utf8ClusterAssembler::NonCjkKind classifyNonCjk(uint32_t cp) {
  using NonCjkKind = Utf8ClusterAssembler::NonCjkKind;
  if (cp == 0x20 || cp == 0x09 || cp == 0x0A || cp == 0x0D) return NonCjkKind::Whitespace;
  if (cp == 0x00A0 || cp == 0x202F) return NonCjkKind::Nbsp;
  if (cp == 0xFEFF) return NonCjkKind::Feff;
  return NonCjkKind::Latin;
}

// Copy `state`'s staged base into `out`. Does NOT clear the staged base.
void fillFlushableFromState(const Utf8ClusterAssembler::State& state, Utf8ClusterAssembler::Flushable& out) {
  memcpy(out.bytes, state.pendingCjkBase, state.pendingCjkBaseLen);
  out.len = state.pendingCjkBaseLen;
  out.join = state.pendingCjkBaseJoin;
  out.fontStyle = state.fontStyleAtStage;
  out.underline = state.underlineAtStage;
}

}  // namespace

Utf8ClusterAssembler::ConsumeResult Utf8ClusterAssembler::tryConsumeCodepoint(
    const char* s, int len, int& i, State& state, WordJoin& nextJoin, EpdFontFamily::Style callerFontStyle,
    bool callerUnderline, Flushable& outFlushable, NonCjkKind& outNonCjkKind, uint32_t& outNonCjkCp,
    uint8_t& outNonCjkLen) {
  // --- Step 1/2: assemble a NUL-terminated scratch holding exactly one codepoint. ---
  // utf8NextCodepoint stops on a NUL, so we MUST decode from this private scratch and never
  // off s[] (which is not NUL-terminated).
  char scratch[5];
  uint8_t scratchLen = 0;       // bytes placed into scratch (full cp length)
  uint8_t continuationBytes = 0;  // bytes pulled from THIS call's s[] (drives i advance)

  if (state.pendingUtf8Len > 0) {
    // Completing a codepoint split across callbacks.
    const uint8_t expected = leadByteLen(state.pendingUtf8[0]);
    memcpy(scratch, state.pendingUtf8, state.pendingUtf8Len);
    scratchLen = state.pendingUtf8Len;

    while (scratchLen < expected && (i + continuationBytes) < len) {
      scratch[scratchLen++] = s[i + continuationBytes];
      continuationBytes++;
    }

    if (scratchLen < expected) {
      // Still incomplete: stage what we pulled and signal NeedMore. i unchanged.
      memcpy(state.pendingUtf8, scratch, scratchLen);
      state.pendingUtf8Len = scratchLen;
      return ConsumeResult::NeedMore;
    }
    state.pendingUtf8Len = 0;  // carry-over consumed
  } else {
    if (i >= len) return ConsumeResult::NeedMore;  // nothing to read (defensive)
    const uint8_t expected = leadByteLen(static_cast<uint8_t>(s[i]));
    if (i + expected > len) {
      // Partial sequence at the buffer tail — stage and signal NeedMore. i unchanged.
      for (uint8_t b = 0; b < expected && (i + b) < len; ++b) state.pendingUtf8[b] = s[i + b];
      state.pendingUtf8Len = static_cast<uint8_t>(len - i);
      return ConsumeResult::NeedMore;
    }
    scratchLen = expected;
    continuationBytes = expected;
    memcpy(scratch, s + i, expected);
  }
  scratch[scratchLen] = '\0';

  const unsigned char* p = reinterpret_cast<const unsigned char*>(scratch);
  const uint32_t cp = utf8NextCodepoint(&p);
  const uint8_t cpLen = scratchLen;  // full UTF-8 byte length of this codepoint

  // --- Step 3: classify — grapheme extender FIRST, then CJK breakable, then non-CJK. ---

  // Grapheme extender glued onto an existing staged base.
  if (utf8IsGraphemeExtender(cp) && state.pendingCjkBaseLen > 0) {
    // Step 4: overflow guard — if appending would exceed the staged buffer, force-flush the
    // staged base and re-stage the extender as a NEW base with a reset join + fresh snapshot.
    if (static_cast<size_t>(state.pendingCjkBaseLen) + cpLen > sizeof(state.pendingCjkBase)) {
      fillFlushableFromState(state, outFlushable);
      memcpy(state.pendingCjkBase, scratch, cpLen);
      state.pendingCjkBaseLen = cpLen;
      state.pendingCjkBaseJoin = WordJoin::CjkBreak;  // explicit reset, NOT the inherited join
      state.fontStyleAtStage = callerFontStyle;
      state.underlineAtStage = callerUnderline;
      i += continuationBytes;
      return ConsumeResult::EmittedFlushable;
    }
    memcpy(state.pendingCjkBase + state.pendingCjkBaseLen, scratch, cpLen);
    state.pendingCjkBaseLen = static_cast<uint8_t>(state.pendingCjkBaseLen + cpLen);
    i += continuationBytes;
    return ConsumeResult::StagedOnly;
  }

  // CJK breakable base.
  if (utf8IsCjkBreakable(cp)) {
    const bool hadBase = state.pendingCjkBaseLen > 0;
    if (hadBase) fillFlushableFromState(state, outFlushable);
    // Stage the NEW base.
    memcpy(state.pendingCjkBase, scratch, cpLen);
    state.pendingCjkBaseLen = cpLen;
    state.pendingCjkBaseJoin = nextJoin;
    state.fontStyleAtStage = callerFontStyle;
    state.underlineAtStage = callerUnderline;
    nextJoin = WordJoin::CjkBreak;
    i += continuationBytes;
    return hadBase ? ConsumeResult::EmittedFlushable : ConsumeResult::StagedOnly;
  }

  // Non-CJK: Latin / whitespace / NBSP / FEFF. Do NOT touch nextJoin — the caller sets it.
  outNonCjkKind = classifyNonCjk(cp);
  outNonCjkCp = cp;
  outNonCjkLen = cpLen;
  if (state.pendingCjkBaseLen > 0) {
    fillFlushableFromState(state, outFlushable);
    state.pendingCjkBaseLen = 0;  // clear staged base
    i += continuationBytes;
    return ConsumeResult::EmittedAndNonCjk;
  }
  i += continuationBytes;
  return ConsumeResult::NonCjkOnly;
}

bool Utf8ClusterAssembler::flushPendingBase(State& state, Flushable& outFlushable) {
  if (state.pendingCjkBaseLen == 0) return false;
  fillFlushableFromState(state, outFlushable);
  state.pendingCjkBaseLen = 0;
  return true;
}
