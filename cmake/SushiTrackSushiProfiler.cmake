# SushiTrackSushiProfiler.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# SushiProfiler, which the zones, counters and instants of profiling.hpp record into. The CLI
# builds or reuses its package and passes SushiProfiler_DIR; nothing is built in this tree and
# nothing is downloaded. With the option off no SushiProfiler header is included.

option(SUSHITRACK_ENABLE_PROFILER
    "Record zones, counters and instants of Tracker::update through SushiProfiler" ON)

# Empty means the default: 2 in a Debug configuration and 1 in every other. The level is set
# explicitly on every target, so the library and its consumers agree whatever NDEBUG says.
set(SUSHITRACK_PROFILE_LEVEL "" CACHE STRING
    "Deepest profiling level compiled in: empty for 1 (2 in Debug), or 0, 1, 2")
set_property(CACHE SUSHITRACK_PROFILE_LEVEL PROPERTY STRINGS "" 0 1 2)
if(NOT SUSHITRACK_PROFILE_LEVEL MATCHES "^[0-2]?$")
    message(FATAL_ERROR
        "SUSHITRACK_PROFILE_LEVEL is '${SUSHITRACK_PROFILE_LEVEL}'; it must be empty, 0, 1 or 2")
endif()

if(SUSHITRACK_ENABLE_PROFILER)
    find_package(SushiProfiler CONFIG REQUIRED)
    message(STATUS "Found SushiProfiler ${SushiProfiler_VERSION}: ${SushiProfiler_DIR}")
endif()

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

# Applies the profiling switch, the level and the SushiProfiler link to <target> with the given
# scope. The version and the commit reach only sushitrack_profiler_parts.cpp, so a new commit
# recompiles that file and no other.
function(sushitrack_apply_profiling target scope)
    if(SUSHITRACK_PROFILE_LEVEL STREQUAL "")
        set(level "$<IF:$<CONFIG:Debug>,2,1>")
    else()
        set(level "${SUSHITRACK_PROFILE_LEVEL}")
    endif()
    if(SUSHITRACK_ENABLE_PROFILER)
        target_compile_definitions(${target} ${scope}
            SUSHITRACK_ENABLE_PROFILER=1 SUSHITRACK_PROFILE_LEVEL=${level})
        target_link_libraries(${target} ${scope} SushiProfiler::SushiProfiler)
    else()
        target_compile_definitions(${target} ${scope} SUSHITRACK_ENABLE_PROFILER=0)
    endif()
    set_property(SOURCE "${PROJECT_SOURCE_DIR}/source/profiling/sushitrack_profiler_parts.cpp"
        APPEND PROPERTY COMPILE_DEFINITIONS
        "SUSHITRACK_VERSION_STRING=\"${PROJECT_VERSION}\""
        "SUSHITRACK_COMMIT=\"${SUSHITRACK_COMMIT_ID}\"")
endfunction()
