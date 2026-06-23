# ==============================================================================
# integration_test — GTest-based end-to-end integration tests.
#
# Build:
#   sushitrack project build --integration
#
# Run via CLI:
#   sushitrack project test integration
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
    TestInteractionContext.cpp
    TestDuplicateSuppression.cpp
)

target_include_directories(integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty
    ${CMAKE_SOURCE_DIR}/3rdparty/eigen
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
)
