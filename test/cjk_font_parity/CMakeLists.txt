# test/cjk_font_parity/CMakeLists.txt
#
# The parity test compares the converted EpdFontData against the fork's reference
# data extracted via the same Python parser used by the converter. This avoids
# include-shimming the fork header (which would require deleting both the namespace
# opening `{` and its matching closing `}` — fragile against any future header
# layout change).
#
# The fork source lives outside the repo (it is not checked in). Where it is
# absent — e.g. the standard CI runner — this is a local-only verification, so the
# target is skipped rather than failing the build. CI's drift guard is the
# `sha256:` comment in the committed cjk_ui_20.{h,cpp} artifact.

set(FORK_HEADER /tmp/crosspoint-jp/lib/GfxRenderer/cjk_ui_font_20.h)

if(NOT EXISTS ${FORK_HEADER})
  message(STATUS "CJK fork reference not found at ${FORK_HEADER}; skipping cjk_font_parity (local-only test)")
  return()
endif()

set(REF_HEADER ${CMAKE_CURRENT_BINARY_DIR}/cjk_ui_20_parity_ref.h)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(PYTHON3 ${Python3_EXECUTABLE})

add_custom_command(
  OUTPUT ${REF_HEADER}
  COMMAND ${PYTHON3} ${REPO_ROOT}/scripts/extract_cjk_fork_reference.py
          --input ${FORK_HEADER}
          --output ${REF_HEADER}
  DEPENDS ${FORK_HEADER}
          ${REPO_ROOT}/scripts/extract_cjk_fork_reference.py
          ${REPO_ROOT}/scripts/convert_cjk_font_to_epdfontdata.py
  COMMENT "Extracting CJK fork reference data for parity test"
)

add_executable(CjkFontParityTest
  CjkFontParityTest.cpp
  ${REF_HEADER}
  ${REPO_ROOT}/lib/EpdFont/builtinFonts/cjk_ui_20.cpp
)
target_include_directories(CjkFontParityTest PRIVATE
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/lib/EpdFont/builtinFonts
  ${REPO_ROOT}/test/support
  ${CMAKE_CURRENT_BINARY_DIR})  # for cjk_ui_20_parity_ref.h
target_link_libraries(CjkFontParityTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(CjkFontParityTest)
