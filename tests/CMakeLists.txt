# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

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

        # A header with no runtime promise keeps compile_<name>.cpp, whose static_asserts are
        # its test: the build of this object library is the check.
        file(GLOB_RECURSE SUSHITRACK_APPLICATION_COMPILE_CHECKS CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/applications/*/compile_*.cpp")
        if(SUSHITRACK_APPLICATION_COMPILE_CHECKS)
            add_library(sushitrack_application_compile_checks OBJECT
                ${SUSHITRACK_APPLICATION_COMPILE_CHECKS})
            target_compile_features(sushitrack_application_compile_checks PRIVATE cxx_std_17)
            target_link_libraries(sushitrack_application_compile_checks PRIVATE
                mot_engine_runner tracker_bridge)
        endif()
    endif()
endif()
