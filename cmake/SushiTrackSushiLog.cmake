# SushiTrackSushiLog.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Makes SushiLog::SushiLog exist from the SUSHILOG_DIR the CLI resolves; nothing is downloaded.

option(SUSHITRACK_ENABLE_LOGGING
    "Compile SushiTrack log calls below ERROR (TRACE in a debug build, DEBUG under NDEBUG)" ON)

if(NOT TARGET SushiLog::SushiLog)
    if(SUSHILOG_DIR AND EXISTS "${SUSHILOG_DIR}/CMakeLists.txt")
        set(SL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${SUSHILOG_DIR}" "${PROJECT_BINARY_DIR}/sushilog")
    else()
        find_package(SushiLog CONFIG HINTS "${SUSHILOG_DIR}")
    endif()
endif()
if(NOT TARGET SushiLog::SushiLog)
    message(FATAL_ERROR
        "SushiLog was not found (SUSHILOG_DIR='${SUSHILOG_DIR}'). Run `st setup`, which fetches "
        "the version cli/sushistack.deps.toml pins, or set SUSHILOG_DIR to a SushiLog checkout "
        "or install prefix.")
endif()
message(STATUS "Found SushiLog: ${SUSHILOG_DIR}")

# Applies SUSHITRACK_ENABLE_LOGGING to <target> with the given scope.
function(sushitrack_apply_logging target scope)
    if(NOT SUSHITRACK_ENABLE_LOGGING)
        target_compile_definitions(${target} ${scope} SUSHITRACK_ENABLE_LOGGING=0)
    endif()
endfunction()
