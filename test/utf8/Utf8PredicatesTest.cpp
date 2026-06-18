#include <gtest/gtest.h>

#include <Utf8.h>

TEST(Utf8Predicates, CjkBreakableSupplementaryRanges) {
  EXPECT_TRUE(utf8IsCjkBreakable(0x30000));   // CJK Extension G first codepoint
  EXPECT_TRUE(utf8IsCjkBreakable(0x1B000));   // Kana Supplement representative
  EXPECT_TRUE(utf8IsCjkBreakable(0x1B100));   // Kana Extended-A first codepoint
  EXPECT_TRUE(utf8IsCjkBreakable(0x2B740));   // CJK Extension D first codepoint
  EXPECT_TRUE(utf8IsCjkBreakable(0x2F800));   // CJK Compatibility Ideographs Supplement
  EXPECT_TRUE(utf8IsCjkBreakable(0x31350));   // CJK Extension H first codepoint (Unicode 15.0)
  EXPECT_FALSE(utf8IsCjkBreakable(0x0061));   // ASCII 'a' — must not match
}

TEST(Utf8Predicates, GraphemeExtenderCoversAllOverlapRanges) {
  EXPECT_TRUE(utf8IsGraphemeExtender(0x3099));   // Combining hiragana-katakana voicing
  EXPECT_TRUE(utf8IsGraphemeExtender(0x309A));   // Combining hiragana-katakana semi-voicing
  EXPECT_TRUE(utf8IsGraphemeExtender(0x302A));   // Ideographic tone mark (left)
  EXPECT_TRUE(utf8IsGraphemeExtender(0x302F));   // Ideographic tone mark range end
  EXPECT_TRUE(utf8IsGraphemeExtender(0xFF9E));   // Halfwidth voicing
  EXPECT_TRUE(utf8IsGraphemeExtender(0xFF9F));   // Halfwidth semi-voicing
  EXPECT_TRUE(utf8IsGraphemeExtender(0xFE00));   // Variation Selector VS-1
  EXPECT_TRUE(utf8IsGraphemeExtender(0xFE0F));   // Variation Selector VS-16
  EXPECT_TRUE(utf8IsGraphemeExtender(0xE0100));  // Variation Selectors Supplement first
  EXPECT_TRUE(utf8IsGraphemeExtender(0xE01EF));  // Variation Selectors Supplement last
  EXPECT_FALSE(utf8IsGraphemeExtender(0x0061));  // ASCII 'a' — must not match
  EXPECT_FALSE(utf8IsGraphemeExtender(0x65E5));  // 日 — base ideograph, must not match
}
