add_executable(FontBoundaryTest
  FontBoundaryTest.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFontFamily.cpp
  ${REPO_ROOT}/lib/EpdFont/builtinFonts/cjk_ui_20.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp)

target_link_libraries(FontBoundaryTest PRIVATE crosspoint_test_common GTest::gtest_main)

target_include_directories(FontBoundaryTest PRIVATE
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/lib/EpdFont/builtinFonts
  ${REPO_ROOT}/lib/Utf8
  ${REPO_ROOT}/test/support)

gtest_discover_tests(FontBoundaryTest)
