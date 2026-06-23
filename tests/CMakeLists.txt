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
# Both targets register with CTest. Run selectively:
#   ctest -R unit              -- unit tests only
#   ctest -R RegressionTest    -- regression tests only
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
