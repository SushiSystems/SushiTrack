# SushiTrackSushiLog.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# SushiLog, the one logger of the process. The CLI builds or reuses its package and passes
# SushiLog_DIR; nothing is built in this tree and nothing is downloaded.

option(SUSHITRACK_ENABLE_LOGGING
    "Compile SushiTrack log calls below ERROR (TRACE in a debug build, DEBUG under NDEBUG)" ON)

find_package(SushiLog CONFIG REQUIRED)
message(STATUS "Found SushiLog ${SushiLog_VERSION}: ${SushiLog_DIR}")

# Applies SUSHITRACK_ENABLE_LOGGING to <target> with the given scope.
function(sushitrack_apply_logging target scope)
    if(NOT SUSHITRACK_ENABLE_LOGGING)
        target_compile_definitions(${target} ${scope} SUSHITRACK_ENABLE_LOGGING=0)
    endif()
endfunction()
