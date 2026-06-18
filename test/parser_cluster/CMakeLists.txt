# test/parser_cluster/CMakeLists.txt
add_executable(ParserClusterTest
  ParserClusterTest.cpp
  ${REPO_ROOT}/lib/Epub/Epub/parsers/Utf8ClusterAssembler.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp
)
target_include_directories(ParserClusterTest PRIVATE
  ${REPO_ROOT}
  ${REPO_ROOT}/lib/Utf8
  ${REPO_ROOT}/lib/Epub
  ${REPO_ROOT}/lib/Epub/Epub
  ${REPO_ROOT}/lib/Epub/Epub/parsers
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/test
)
target_link_libraries(ParserClusterTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(ParserClusterTest)
