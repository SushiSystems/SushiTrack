# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# ==============================================================================
# regression_test — MOT label-based benchmark runner.
#
# Build:
#   cmake -DBUILD_REGRESSION_TEST=ON ..
#   cmake --build . --target regression_test
#
# Run via CTest:
#   st test --suite regression  (pass/fail; or: ctest --test-dir build -L '^regression$')
#   st eval                     (the same binary, plus TrackEval metrics)
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
    ${CMAKE_SOURCE_DIR}/third_party/bytetrack/include
    ${CMAKE_SOURCE_DIR}/third_party/ocsort/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/eigen
)

# Linked against sushitrack_static, not the SHARED sushitrack target: the
# TestInfrastructure::SushiTrackAdapter this links (SushiTrackAdapter.h)
# constructs SushiTrack::Object/Rect<float> directly, neither of which is
# part of the public C ABI's export surface. See tests/unit/CMakeLists.txt's
# comment and the root CMakeLists.txt's sushitrack_static target.
target_link_libraries(regression_test PRIVATE
    sushitrack_static
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
    ${CMAKE_SOURCE_DIR}/third_party/bytetrack/include
    ${CMAKE_SOURCE_DIR}/third_party/ocsort/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/eigen
)

# See regression_test's comment above: statically linked for the same
# reason. tracker_bridge's own export surface is unaffected -- only its
# EXPORT_API-tagged extern "C" functions (TrackerRegistry.h) are exported;
# the statically-linked SushiTrack code stays internal to tracker_bridge.dll,
# compiled hidden-by-default like every other target here.
target_link_libraries(tracker_bridge PRIVATE
    sushitrack_static
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
