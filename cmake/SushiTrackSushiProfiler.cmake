# SushiTrackSushiProfiler.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# SushiProfiler, which the zones, counters and instants of the library's private profiling.hpp
# record into. The CLI builds or reuses its package and passes SushiProfiler_DIR; nothing is built
# in this tree and nothing is downloaded. No installed header includes a SushiProfiler header, and
# the switch and the level reach the code through a generated profile_config.hpp, not through
# compile definitions.

option(SUSHITRACK_ENABLE_PROFILER
    "Record zones, counters and instants of Tracker::update through SushiProfiler" ON)

set(SUSHITRACK_PROFILE_LEVEL 1 CACHE STRING
    "Deepest profiling level compiled in: 0 none, 1 stages and counters, 2 fine zones")
set_property(CACHE SUSHITRACK_PROFILE_LEVEL PROPERTY STRINGS 0 1 2)
if(NOT SUSHITRACK_PROFILE_LEVEL MATCHES "^[0-2]$")
    message(FATAL_ERROR "SUSHITRACK_PROFILE_LEVEL is '${SUSHITRACK_PROFILE_LEVEL}'; it must be 0, 1 or 2")
endif()

if(SUSHITRACK_ENABLE_PROFILER)
    set(SUSHITRACK_ENABLE_PROFILER_VALUE 1)
    find_package(SushiProfiler CONFIG REQUIRED)
    message(STATUS "Found SushiProfiler ${SushiProfiler_VERSION}: ${SushiProfiler_DIR}")
else()
    set(SUSHITRACK_ENABLE_PROFILER_VALUE 0)
endif()

set(SUSHITRACK_GENERATED_DIR "${PROJECT_BINARY_DIR}/generated")
configure_file("${PROJECT_SOURCE_DIR}/source/profiling/profile_config.hpp.in"
    "${SUSHITRACK_GENERATED_DIR}/SushiTrack/profiling/profile_config.hpp" @ONLY)

# The commit the record names as its build. A source without a git checkout reports "unknown".
set(SUSHITRACK_COMMIT_ID "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE git_head
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE git_status
        ERROR_QUIET)
    if(git_status EQUAL 0 AND git_head)
        set(SUSHITRACK_COMMIT_ID "${git_head}")
    endif()
endif()

# Gives <target> the private profiling headers, the generated config and, when the option is on,
# SushiProfiler, all PRIVATE. The library, its test programs and the mot_engine runner call it.
function(sushitrack_link_profiling target)
    target_include_directories(${target} PRIVATE
        "${PROJECT_SOURCE_DIR}/source/private_include"
        "${SUSHITRACK_GENERATED_DIR}")
    if(SUSHITRACK_ENABLE_PROFILER)
        target_link_libraries(${target} PRIVATE SushiProfiler::SushiProfiler)
    endif()
endfunction()

# Applies the profiling build to a library target: the private headers, the link and the version
# and commit of the record. The version and the commit reach only sushitrack_profiler_parts.cpp,
# so a new commit recompiles that file and no other.
function(sushitrack_apply_profiling target)
    sushitrack_link_profiling(${target})
    set_property(SOURCE "${PROJECT_SOURCE_DIR}/source/profiling/sushitrack_profiler_parts.cpp"
        APPEND PROPERTY COMPILE_DEFINITIONS
        "SUSHITRACK_VERSION_STRING=\"${PROJECT_VERSION}\""
        "SUSHITRACK_COMMIT=\"${SUSHITRACK_COMMIT_ID}\"")
endfunction()
