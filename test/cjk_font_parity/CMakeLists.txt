# test/cjk_font_parity/CMakeLists.txt
#
# Two host test executables guard the converted CJK font asset:
#
#   CjkFontSelfTest   — fork-INDEPENDENT structural invariants of the committed
#                       cjk_ui_20.{h,cpp} (metrics, per-glyph metadata, bitmap
#                       offset contiguity, interval table). Built unconditionally,
#                       so CI verifies the artifact on every run.
#
#   CjkFontParityTest — fork-DEPENDENT bit-per-bit pixel parity vs the fork's
#                       cjk_ui_font_20.h. The fork source is not checked in, so
#                       this target is built only where it is present (local dev)
#                       and skipped elsewhere (e.g. standard CI).
#
# The `// sha256:` comment in the generated artifact is a marker for detecting
# drift against the fork source by re-running the converter; it is NOT an
# automated CI check. Full pixel parity is therefore a local gate; CI's standing
# protection is CjkFontSelfTest.

# --- Fork-independent self-consistency test (always built) ---
add_executable(CjkFontSelfTest
  CjkFontSelfTest.cpp
  ${REPO_ROOT}/lib/EpdFont/builtinFonts/cjk_ui_20.cpp
)
target_include_directories(CjkFontSelfTest PRIVATE
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/lib/EpdFont/builtinFonts
  ${REPO_ROOT}/test/support)
target_link_libraries(CjkFontSelfTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(CjkFontSelfTest)

# --- Fork-dependent pixel-parity test (local-only) ---
set(FORK_HEADER /tmp/crosspoint-jp/lib/GfxRenderer/cjk_ui_font_20.h)

if(NOT EXISTS ${FORK_HEADER})
  message(STATUS "CJK fork reference not found at ${FORK_HEADER}; skipping CjkFontParityTest (local-only pixel parity)")
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
