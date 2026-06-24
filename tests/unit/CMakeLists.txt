# ==============================================================================
# unit_test — GTest-based deterministic unit tests for SushiTrack components.
#
# Build:
#   cmake -DBUILD_UNIT_TEST=ON ..
#   cmake --build . --target unit_test
#
# Run via CTest:
#   ctest -R unit --output-on-failure
#
# Run directly:
#   ./unit_test
#   ./unit_test --gtest_filter="TrackFSMTest.*"
# ==============================================================================

find_package(GTest REQUIRED)

add_executable(unit_test
    TestTrackFSM.cpp
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
gtest_discover_tests(unit_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
