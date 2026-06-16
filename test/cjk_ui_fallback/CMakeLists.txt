add_executable(CjkUiFallbackTest
  CjkUiFontTest.cpp
  EpdFontDispatchTest.cpp
  ${REPO_ROOT}/lib/EpdFont/EpdFont.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp)
target_link_libraries(CjkUiFallbackTest PRIVATE crosspoint_test_common GTest::gtest_main)
target_include_directories(CjkUiFallbackTest PRIVATE
  ${REPO_ROOT}/lib/EpdFont ${REPO_ROOT}/lib/Utf8 ${REPO_ROOT}/test/support)
gtest_discover_tests(CjkUiFallbackTest)
