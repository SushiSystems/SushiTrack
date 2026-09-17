# ==============================================================================
# integration_test — GTest-based end-to-end integration tests.
#
# Build:
#   ds build
#
# Run via CLI:
#   ds test --suite integration
# ==============================================================================

find_package(GTest REQUIRED)

add_executable(integration_test
    Main.cpp
    TestAdvancedPath.cpp
    TestOcclusionRecovery.cpp
    TestByteAssociation.cpp
    TestMultiObject.cpp
    TestConfigVariations.cpp
    TestFullStack.cpp
    TestTimeAware.cpp
    TestKinematicState.cpp
    TestDuplicateSuppression.cpp
    TestObservationMomentum.cpp
)

target_include_directories(integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/eigen
)

target_link_libraries(integration_test PRIVATE
    sushitrack
    GTest::GTest
)

set_target_properties(integration_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(integration_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    PROPERTIES LABELS "integration"
)
