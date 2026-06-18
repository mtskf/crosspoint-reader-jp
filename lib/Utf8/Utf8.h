#pragma once

#include <cstdint>
#include <string>
#define REPLACEMENT_GLYPH 0xFFFD

uint32_t utf8NextCodepoint(const unsigned char** string);
// Appends a Unicode codepoint to a std::string in UTF-8 encoding.
void utf8AppendCodepoint(uint32_t cp, std::string& out);
// Remove the last UTF-8 codepoint from a std::string and return the new size.
size_t utf8RemoveLastChar(std::string& str);
// Truncate string by removing N UTF-8 codepoints from the end.
void utf8TruncateChars(std::string& str, size_t numChars);

// Truncate a raw char buffer to the last complete UTF-8 codepoint boundary.
// Returns the new length (<= len). If the buffer ends mid-sequence, the
// incomplete trailing bytes are excluded.
int utf8SafeTruncateBuffer(const char* buf, int len);

// Returns true for CJK characters that allow line breaks on either side without hyphenation.
// Covers CJK Unified Ideographs, Hiragana, Katakana, Hangul Syllables, CJK punctuation,
// and fullwidth forms — the ranges where word boundaries are implicit per character.
inline bool utf8IsCjkBreakable(const uint32_t cp) {
  return (cp >= 0x3000 && cp <= 0x303F)        // CJK Symbols and Punctuation
         || (cp >= 0x3040 && cp <= 0x309F)     // Hiragana
         || (cp >= 0x30A0 && cp <= 0x30FF)     // Katakana
         || (cp >= 0x3400 && cp <= 0x4DBF)     // CJK Extension A
         || (cp >= 0x4E00 && cp <= 0x9FFF)     // CJK Unified Ideographs
         || (cp >= 0xAC00 && cp <= 0xD7AF)     // Hangul Syllables
         || (cp >= 0xF900 && cp <= 0xFAFF)     // CJK Compatibility Ideographs
         || (cp >= 0xFE30 && cp <= 0xFE4F)     // CJK Compatibility Forms
         || (cp >= 0xFF01 && cp <= 0xFF60)     // Fullwidth Latin / Punctuation
         || (cp >= 0xFF65 && cp <= 0xFFEF)     // Halfwidth Katakana / Hangul
         || (cp >= 0x20000 && cp <= 0x2A6DF)   // CJK Extension B
         || (cp >= 0x2A700 && cp <= 0x2B73F)   // CJK Extension C
         || (cp >= 0x1B000 && cp <= 0x1B0FF)   // Kana Supplement
         || (cp >= 0x1B100 && cp <= 0x1B12F)   // Kana Extended-A
         || (cp >= 0x2B740 && cp <= 0x2B81F)   // CJK Extension D
         || (cp >= 0x2B820 && cp <= 0x2CEAF)   // CJK Extension E
         || (cp >= 0x2CEB0 && cp <= 0x2EBEF)   // CJK Extension F
         || (cp >= 0x2F800 && cp <= 0x2FA1F)   // CJK Compatibility Ideographs Supplement
         || (cp >= 0x30000 && cp <= 0x3134F)   // CJK Extension G
         || (cp >= 0x1AFF0 && cp <= 0x1AFFF)   // Kana Extended-B
         || (cp >= 0x1B130 && cp <= 0x1B16F)   // Small Kana Extension
         || (cp >= 0x2EBF0 && cp <= 0x2EE5F)   // CJK Extension I
         || (cp >= 0x31350 && cp <= 0x323AF)   // CJK Extension H
         || (cp >= 0x323B0 && cp <= 0x3347F);  // CJK Extension J
}

// Returns true for Unicode combining diacritical marks that should not advance the cursor.
inline bool utf8IsCombiningMark(const uint32_t cp) {
  return (cp >= 0x0300 && cp <= 0x036F)      // Combining Diacritical Marks
         || (cp >= 0x1DC0 && cp <= 0x1DFF)   // Combining Diacritical Marks Supplement
         || (cp >= 0x20D0 && cp <= 0x20FF)   // Combining Diacritical Marks for Symbols
         || (cp >= 0xFE20 && cp <= 0xFE2F);  // Combining Half Marks
}

// Returns true for grapheme extenders that must stay glued to a preceding base
// codepoint in layout (combining marks, kana voicing marks, ideographic tone
// marks, variation selectors).
inline bool utf8IsGraphemeExtender(uint32_t cp) {
  return utf8IsCombiningMark(cp)
         || (cp >= 0x3099 && cp <= 0x309A)    // Combining katakana-hiragana voicing
         || (cp >= 0x302A && cp <= 0x302F)    // Ideographic tone marks
         || (cp >= 0xFF9E && cp <= 0xFF9F)    // Halfwidth voicing marks
         || (cp >= 0xFE00 && cp <= 0xFE0F)    // Variation Selectors VS-1..VS-16
         || (cp >= 0xE0100 && cp <= 0xE01EF); // Variation Selectors Supplement
}
