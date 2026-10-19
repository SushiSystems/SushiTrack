# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Builds sushitrack_tests from the mirror of the library and sushitrack_application_tests from
# the mirror of applications/; the size of each suite is its label.

if(SUSHITRACK_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    file(GLOB_RECURSE SUSHITRACK_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*/test_*.cpp")
    list(FILTER SUSHITRACK_TEST_SOURCES EXCLUDE REGEX "/applications/")
    sushitrack_add_sized_test_program(sushitrack_tests
        TEST_SOURCES ${SUSHITRACK_TEST_SOURCES}
        ENVIRONMENT "SUSHITRACK_CONFIG=${PROJECT_SOURCE_DIR}/assets/sushitrack.json")

    if(SUSHITRACK_BUILD_APPLICATIONS)
        file(GLOB_RECURSE SUSHITRACK_APPLICATION_TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/applications/*/test_*.cpp")
        sushitrack_add_sized_test_program(sushitrack_application_tests
            TEST_SOURCES ${SUSHITRACK_APPLICATION_TEST_SOURCES}
            LINK mot_engine_runner tracker_bridge
            ENVIRONMENT "SUSHITRACK_CONFIG=${PROJECT_SOURCE_DIR}/assets/sushitrack.json")
    endif()
endif()
