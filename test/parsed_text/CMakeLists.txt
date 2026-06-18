# IMPORTANT: TextBlock.cpp is intentionally NOT linked here. It pulls <GfxRenderer.h>,
# which chains into HalDisplay (Arduino/HAL). ParsedText only uses TextBlock through
# its header-inline ctor and getWords(), so the cpp body is not required to link.
# If a later refactor moves any of those out-of-line, add a HOST_TEST guard inside
# TextBlock.cpp around the render/serialize bodies and link it here with -DHOST_TEST.
add_executable(ParsedTextLayoutTest
  ParsedTextLayoutTest.cpp
  ${REPO_ROOT}/lib/Epub/Epub/ParsedText.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/Hyphenator.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/LanguageRegistry.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/LiangHyphenation.cpp
  ${REPO_ROOT}/lib/Epub/Epub/hyphenation/HyphenationCommon.cpp
  ${REPO_ROOT}/lib/MiniBidi/BidiUtils.cpp
  ${REPO_ROOT}/lib/MiniBidi/minibidi.c
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)
target_include_directories(ParsedTextLayoutTest PRIVATE
  ${REPO_ROOT}
  ${REPO_ROOT}/lib/Epub
  ${REPO_ROOT}/lib/GfxRenderer
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/lib/Utf8
  ${REPO_ROOT}/lib/MiniBidi
  ${REPO_ROOT}/test
  ${REPO_ROOT}/test/support
)
target_link_libraries(ParsedTextLayoutTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(ParsedTextLayoutTest)
