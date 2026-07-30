# ==============================================================================
# regression_test — MOT label-based benchmark runner.
#
# Build:
#   cmake -DBUILD_REGRESSION_TEST=ON ..
#   cmake --build . --target regression_test
#
# Run via CTest:
#   ds test --suite regression  (pass/fail; or: ctest --test-dir build -L '^regression$')
#   ds eval                     (the same binary, plus TrackEval metrics)
#
# Run directly (all sequences):
#   ./regression_test
#
# Run with a custom config:
#   SUSHITRACK_CONFIG=./sushitrack.json ./regression_test
#
# Run a single tracker:
#   ./regression_test --gtest_filter="MOT/RegressionTest.*sushitrack*"
# ==============================================================================

find_package(GTest REQUIRED)

add_executable(regression_test
    RegressionTest.cpp
    TrackerRegistry.cpp
)

target_include_directories(regression_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty/bytetrack/include
    ${CMAKE_SOURCE_DIR}/3rdparty/ocsort/include
    ${CMAKE_SOURCE_DIR}/3rdparty
    ${CMAKE_SOURCE_DIR}/3rdparty/eigen
)

target_link_libraries(regression_test PRIVATE
    sushitrack
    bytetrack
    libocsort
    GTest::GTest
)

set_target_properties(regression_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)


# ---------------------------------------------------------------------------
# tracker_bridge — C-API shared library for Python ctypes video inference.
#
# The Python pipeline in tests/regression/inference/ loads this DLL to drive
# label and video inference from Python without recompiling the C++ core.
# ---------------------------------------------------------------------------
add_library(tracker_bridge SHARED
    TrackerRegistry.cpp
)

target_include_directories(tracker_bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty/bytetrack/include
    ${CMAKE_SOURCE_DIR}/3rdparty/ocsort/include
    ${CMAKE_SOURCE_DIR}/3rdparty
    ${CMAKE_SOURCE_DIR}/3rdparty/eigen
)

target_link_libraries(tracker_bridge PRIVATE
    sushitrack
    bytetrack
    libocsort
)


# ---------------------------------------------------------------------------
# CTest integration
# ---------------------------------------------------------------------------
enable_testing()
include(GoogleTest)
gtest_discover_tests(regression_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    PROPERTIES LABELS "regression"
)
