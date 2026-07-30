# ==============================================================================
# unit_test — GTest-based deterministic unit tests for SushiTrack components.
#
# Build:
#   ds build                    (or: cmake -DBUILD_UNIT_TEST=ON .. && cmake --build . --target unit_test)
#
# Run via CTest (registered under the "unit" label):
#   ds test --suite unit        (or: ctest --test-dir build -L '^unit$' --output-on-failure)
#
# Run directly:
#   ds run unit_test -- --gtest_filter="TrackletTest.*"
# ==============================================================================

find_package(GTest REQUIRED)

add_executable(unit_test
    TestKalmanFilter.cpp
    TestTracklet.cpp
    TestRect.cpp
    TestReID.cpp
    TestCostMatrix.cpp
    TestTimingPolicies.cpp
    TestDistanceStrategies.cpp
    TestAssignmentSolvers.cpp
    TestTrackerTentative.cpp
    TestTrackerBuffers.cpp
    TestLogger.cpp
    TestSushiTrackC.cpp
    TestSushiTrackTracker.cpp
    TestCAPIErrors.cpp
    TestIntegration.cpp
    TestBoundaryConditions.cpp
    TestConfiguration.cpp
    UnitTest.cpp
)

target_include_directories(unit_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty
    ${CMAKE_SOURCE_DIR}/3rdparty/eigen
)

target_link_libraries(unit_test PRIVATE
    sushitrack
    GTest::GTest
)

set_target_properties(unit_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

enable_testing()
include(GoogleTest)
# The "unit" label is what `ds test --suite unit` selects (ctest -L). One label
# per target: the CLI expresses umbrellas as a label regex instead, so nothing
# here has to change when a suite grouping changes.
gtest_discover_tests(unit_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    PROPERTIES LABELS "unit"
)
