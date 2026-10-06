# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# ==============================================================================
# tests/ — Top-level test directory.
#
# Provides two independent CMake targets:
#
#   unit_test        Deterministic GTest unit tests for SushiTrack components.
#                    No external data or shared libraries beyond sushitrack.
#                    Enable: -DBUILD_UNIT_TEST=ON
#
#   regression_test  MOT label-based benchmark suite and video inference bridge.
#                    Requires det.txt datasets under tests/regression/evaluator/data/ and the
#                    ByteTrack/OC-SORT third-party libraries.
#                    Enable: -DBUILD_REGRESSION_TEST=ON
#
# Every target registers with CTest under a single label (unit / integration /
# regression), set on its gtest_discover_tests call. Select by label:
#   ctest --test-dir build -L '^unit$'                 -- unit tests only
#   ctest --test-dir build -L '^(unit|integration)$'   -- an umbrella
#   ctest --test-dir build --print-labels              -- what is registered
# Or through the CLI: `st test --suite unit|integration|regression|all`.
# ==============================================================================

option(BUILD_UNIT_TEST       "Build deterministic GTest unit tests"          OFF)
option(BUILD_REGRESSION_TEST "Build MOT label-based regression test runner"  OFF)
option(BUILD_INTEGRATION_TEST "Build integration tests"                      OFF)

# ---------------------------------------------------------------------------
# Legacy alias: BUILD_MOT_TEST still works, maps to BUILD_REGRESSION_TEST.
# Remove after all CI scripts have migrated.
# ---------------------------------------------------------------------------
if(BUILD_MOT_TEST AND NOT BUILD_REGRESSION_TEST)
    message(STATUS
        "[tests] BUILD_MOT_TEST is deprecated — "
        "setting BUILD_REGRESSION_TEST=ON automatically. "
        "Update your cmake invocation to -DBUILD_REGRESSION_TEST=ON.")
    set(BUILD_REGRESSION_TEST ON CACHE BOOL "" FORCE)
endif()

if(BUILD_UNIT_TEST)
    message(STATUS "[tests] unit_test target enabled")
    add_subdirectory(unit)
endif()

if(BUILD_REGRESSION_TEST)
    message(STATUS "[tests] regression_test target enabled")
    add_subdirectory(regression)
endif()

if(BUILD_INTEGRATION_TEST)
    message(STATUS "[tests] integration_test target enabled")
    add_subdirectory(integration)
endif()
