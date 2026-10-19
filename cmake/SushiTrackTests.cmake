# SushiTrackTests.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Builds the test program from the mirror under tests/ and registers its tests by size: the
# Small_, Medium_ or Large_ prefix of each GoogleTest suite gives its label and its timeout.

set(SUSHITRACK_TEST_SIZES "Small_|small|60" "Medium_|medium|300" "Large_|large|900")

# Sets <out_var> to the GoogleTest suite names the given sources declare.
function(sushitrack_test_suites_of out_var)
    set(suites "")
    foreach(source IN LISTS ARGN)
        file(READ "${source}" text)
        string(REGEX MATCHALL "TEST(_F|_P)?[ \t]*[(][ \t\r\n]*[A-Za-z0-9_]+" uses "${text}")
        foreach(use IN LISTS uses)
            string(REGEX REPLACE "^[^(]*[(][ \t\r\n]*" "" suite "${use}")
            list(APPEND suites "${suite}")
        endforeach()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
    endforeach()
    list(REMOVE_DUPLICATES suites)
    set(${out_var} "${suites}" PARENT_SCOPE)
endfunction()

# Builds <target> from TEST_SOURCES and registers each size with its label and timeout. Each
# name in QUARANTINED, a GoogleTest `Suite.Case` name, also carries the label `quarantined`.
#
#   sushitrack_add_sized_test_program(<target> [QUARANTINED <Suite.Case>...]
#       TEST_SOURCES <file>... [LINK <target>...] [ENVIRONMENT <VAR=value>...])
#
# LINK names the libraries under test; sushitrack_static when none is given.
function(sushitrack_add_sized_test_program target)
    cmake_parse_arguments(ARG "" "" "QUARANTINED;TEST_SOURCES;LINK;ENVIRONMENT" ${ARGN})
    if(NOT ARG_LINK)
        set(ARG_LINK sushitrack_static)
    endif()
    sushitrack_test_suites_of(suites ${ARG_TEST_SOURCES})
    foreach(suite IN LISTS suites)
        if(NOT suite MATCHES "^(Small|Medium|Large)_")
            message(FATAL_ERROR
                "${target}: the suite ${suite} has no size; name it Small_, Medium_ or Large_.")
        endif()
    endforeach()

    add_executable(${target} ${ARG_TEST_SOURCES})
    target_link_libraries(${target} PRIVATE ${ARG_LINK} GTest::gtest_main)
    sushitrack_apply_sanitizers(${target})
    target_include_directories(${target} PRIVATE
        ${PROJECT_SOURCE_DIR}/tests
        ${PROJECT_SOURCE_DIR}/third_party)

    foreach(size IN LISTS SUSHITRACK_TEST_SIZES)
        string(REPLACE "|" ";" parts "${size}")
        list(GET parts 0 prefix)
        list(GET parts 1 label)
        list(GET parts 2 timeout)

        set(quarantined "")
        foreach(name IN LISTS ARG_QUARANTINED)
            if(name MATCHES "^${prefix}")
                list(APPEND quarantined "${name}")
            endif()
        endforeach()
        list(JOIN quarantined ":" quarantined_filter)

        set(filter "${prefix}*:*/${prefix}*")
        if(quarantined_filter)
            string(APPEND filter "-${quarantined_filter}")
            gtest_discover_tests(${target}
                DISCOVERY_MODE PRE_TEST
                WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                TEST_FILTER "${quarantined_filter}"
                PROPERTIES LABELS "${label}\;quarantined" TIMEOUT ${timeout}
                    ENVIRONMENT "${ARG_ENVIRONMENT}")
        endif()
        gtest_discover_tests(${target}
            DISCOVERY_MODE PRE_TEST
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
            TEST_FILTER "${filter}"
            PROPERTIES LABELS "${label}" TIMEOUT ${timeout} ENVIRONMENT "${ARG_ENVIRONMENT}")
    endforeach()
endfunction()
