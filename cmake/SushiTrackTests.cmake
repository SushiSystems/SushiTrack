# SushiTrackTests.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# Builds the test program from the mirror under tests/ and registers its tests by size: the
# Small_, Medium_ or Large_ prefix of each GoogleTest suite gives its label and its timeout per
# case. The cases of a program run in shards per size, <program>.<size>_shard_<i>of<N>.

set(SUSHITRACK_TEST_SIZES "Small_|small|60" "Medium_|medium|300" "Large_|large|900")

# The cases one CTest test runs, per size. A process pays its start-up once, so a case run alone
# is mostly start-up. A shard runs its share of a program's cases in one process: a small case is
# a few milliseconds, so 300 of them take a few seconds; a medium case takes up to 5 s, so 25 keep
# a shard under a minute; a large case takes up to 120 s and runs alone.
set(SUSHITRACK_SHARD_CASES_SMALL 300)
set(SUSHITRACK_SHARD_CASES_MEDIUM 25)
set(SUSHITRACK_SHARD_CASES_LARGE 1)

# Where each shard writes its per-case GoogleTest XML; the time reader reads the folder.
set(SUSHITRACK_GTEST_RESULTS_DIR "${PROJECT_BINARY_DIR}/test_results/gtest")

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

# Counts the cases of <sources> whose suite opens with <prefix> and sets <out_var> to the count.
# A TEST_P or TYPED_TEST_P counts once however many instances it gets, and each INSTANTIATE_*
# adds one for its instance name, so the count is a floor; a shard with no case passes.
function(sushitrack_count_cases out_var prefix)
    set(count 0)
    foreach(source IN LISTS ARGN)
        file(READ "${source}" text)
        string(REGEX MATCHALL
            "(TEST|TEST_F|TEST_P|TYPED_TEST|TYPED_TEST_P)[ \t]*[(][ \t\r\n]*${prefix}[A-Za-z0-9_]*"
            declarations "${text}")
        list(LENGTH declarations declared)
        string(REGEX MATCHALL
            "INSTANTIATE_(TYPED_)?TEST_SUITE_P[ \t]*[(][ \t\r\n]*[A-Za-z0-9_]+[ \t\r\n]*,[ \t\r\n]*${prefix}"
            instances "${text}")
        list(LENGTH instances instantiated)
        math(EXPR count "${count} + ${declared} + ${instantiated}")
    endforeach()
    set(${out_var} ${count} PARENT_SCOPE)
endfunction()

# Registers one CTest test that runs <program> with <filter>, as shard <index> of <shards>.
# GoogleTest splits the filtered cases round-robin by GTEST_TOTAL_SHARDS and GTEST_SHARD_INDEX,
# and the test writes its per-case XML under SUSHITRACK_GTEST_RESULTS_DIR. The remaining
# arguments are the VAR=value pairs of the program's own environment.
function(sushitrack_add_shard_test program name filter labels timeout index shards)
    add_test(NAME ${name}
        COMMAND ${program} --gtest_filter=${filter}
            --gtest_output=xml:${SUSHITRACK_GTEST_RESULTS_DIR}/${name}.xml
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
    set_tests_properties(${name} PROPERTIES
        LABELS "${labels}"
        TIMEOUT ${timeout}
        ENVIRONMENT "GTEST_TOTAL_SHARDS=${shards};GTEST_SHARD_INDEX=${index};${ARGN}")
    # A large shard runs on the real accelerator, of which the resource file CTest gets holds one
    # slot each, so two large shards never share one.
    if("large" IN_LIST labels)
        set_tests_properties(${name} PROPERTIES RESOURCE_GROUPS "gpus:1")
    endif()
endfunction()

# Registers the cases of <program> of one size in shards, with the size's label and a timeout of
# the case timeout times the cases of a shard. <quarantined> is the list of Suite.Case names
# that run apart in one test that also carries `quarantined`; a filter after '-' leaves them out
# of the shards. <size_group> is "<prefix>|<label>|<case timeout>". The remaining arguments are
# the program's environment, then "SOURCES" and the program's sources.
function(sushitrack_register_size_shards program size_group quarantined_names)
    cmake_parse_arguments(ARG "" "" "ENVIRONMENT;SOURCES" ${ARGN})
    string(REPLACE "|" ";" parts "${size_group}")
    list(GET parts 0 prefix)
    list(GET parts 1 label)
    list(GET parts 2 case_timeout)
    string(TOUPPER "${label}" upper)
    sushitrack_count_cases(cases "${prefix}" ${ARG_SOURCES})

    # An instantiated suite is named <instance>/<suite>, so each pattern also carries the slash.
    set(quarantined "")
    set(quarantined_cases 0)
    foreach(name IN LISTS quarantined_names)
        if(name MATCHES "^${prefix}")
            list(APPEND quarantined "${name}" "*/${name}")
            math(EXPR quarantined_cases "${quarantined_cases} + 1")
        endif()
    endforeach()
    set(filter "${prefix}*:*/${prefix}*")
    if(quarantined)
        list(JOIN quarantined ":" quarantined_filter)
        string(APPEND filter "-${quarantined_filter}")
        math(EXPR quarantined_timeout "${case_timeout} * ${quarantined_cases}")
        sushitrack_add_shard_test(${program} ${program}.${label}_quarantined
            "${quarantined_filter}" "${label};quarantined" ${quarantined_timeout} 0 1
            ${ARG_ENVIRONMENT})
        math(EXPR cases "${cases} - ${quarantined_cases}")
    endif()
    if(cases LESS 1)
        return()
    endif()

    math(EXPR shards
        "(${cases} + ${SUSHITRACK_SHARD_CASES_${upper}} - 1) / ${SUSHITRACK_SHARD_CASES_${upper}}")
    math(EXPR shard_cases "(${cases} + ${shards} - 1) / ${shards}")
    math(EXPR timeout "${case_timeout} * ${shard_cases}")
    math(EXPR last "${shards} - 1")
    foreach(index RANGE ${last})
        sushitrack_add_shard_test(${program} ${program}.${label}_shard_${index}of${shards}
            "${filter}" "${label}" ${timeout} ${index} ${shards} ${ARG_ENVIRONMENT})
    endforeach()
endfunction()

# Registers the tests of <program> in shards, one block per size, from SUSHITRACK_TEST_SIZES.
# This is the one function that registers shards.
function(sushitrack_register_sized_tests program quarantined_names)
    foreach(group IN LISTS SUSHITRACK_TEST_SIZES)
        sushitrack_register_size_shards(${program} "${group}" "${quarantined_names}" ${ARGN})
    endforeach()
endfunction()

# Builds <target> from TEST_SOURCES and registers the cases of each size in shards, with its label
# and timeout. Each name in QUARANTINED, a GoogleTest `Suite.Case` name, also carries the label
# `quarantined`.
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
    sushitrack_link_profiling(${target})
    # A Debug tree links GTest's Release build, and vcpkg puts the Debug DLLs beside the program,
    # which crash it on load; the DLLs of the configuration it links are copied over them.
    if(WIN32 AND TARGET GTest::gtest AND TARGET GTest::gtest_main)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:GTest::gtest>"
                "$<TARGET_FILE:GTest::gtest_main>"
                $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying gtest.dll and gtest_main.dll to the test directory")
    endif()
    target_include_directories(${target} PRIVATE
        ${PROJECT_SOURCE_DIR}/tests
        ${PROJECT_SOURCE_DIR}/third_party)

    sushitrack_register_sized_tests(${target} "${ARG_QUARANTINED}"
        ENVIRONMENT ${ARG_ENVIRONMENT} SOURCES ${ARG_TEST_SOURCES})
endfunction()
