add_executable(CjkUiFallbackTest CjkUiFontTest.cpp)
target_link_libraries(CjkUiFallbackTest PRIVATE crosspoint_test_common GTest::gtest_main)
# EpdFont + Utf8 for the headers; test/support for the host <pgmspace.h> stub.
target_include_directories(CjkUiFallbackTest PRIVATE
  ${REPO_ROOT}/lib/EpdFont ${REPO_ROOT}/lib/Utf8 ${REPO_ROOT}/test/support)
gtest_discover_tests(CjkUiFallbackTest)
