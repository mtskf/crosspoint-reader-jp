// lib/Epub/Epub/WordJoin.h
#pragma once

#include <cstdint>

// How the current word joins the previous one in a line:
//   Space    — a space precedes this word; the layout may break here.
//   Glue     — no space and no break (e.g. NBSP, hyphenation remainder bridge).
//   CjkBreak — no space, but the layout MAY break here (the only state CJK needs).
// Predicates are free `constexpr` so call sites read naturally.
enum class WordJoin : uint8_t { Space, Glue, CjkBreak };

constexpr bool needsSpaceBefore(WordJoin j) { return j == WordJoin::Space; }
constexpr bool canBreakBefore(WordJoin j) { return j != WordJoin::Glue; }
