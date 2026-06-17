add_executable(FontResolverTest
  FontResolverTest.cpp
  FontMetricTest.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFontFamily.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp)

target_link_libraries(FontResolverTest PRIVATE crosspoint_test_common GTest::gtest_main)

target_include_directories(FontResolverTest PRIVATE
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/lib/Utf8
  ${REPO_ROOT}/test/support)  # pgmspace.h stub (EpdFont.cpp transitively pulls cjk_ui_font_20.h)

gtest_discover_tests(FontResolverTest)
