# test/utf8/CMakeLists.txt — dedicated predicate target, isolated from ParsedText / assembler.
add_executable(Utf8PredicatesTest
  Utf8PredicatesTest.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)
target_include_directories(Utf8PredicatesTest PRIVATE
  ${REPO_ROOT}
  ${REPO_ROOT}/lib/Utf8
  ${REPO_ROOT}/test
)
target_link_libraries(Utf8PredicatesTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(Utf8PredicatesTest)
