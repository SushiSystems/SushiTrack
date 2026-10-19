# SushiTrackSanitizers.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Instruments the project's own targets with the sanitizer SUSHITRACK_SANITIZE names, which
# `st build --sanitize` sets; docs/architecture/BUILD_SYSTEM.md gives the reasons.

set(SUSHITRACK_SANITIZE "" CACHE STRING
    "Sanitizer build: empty, address (ASan and UBSan) or thread (TSan)")
set_property(CACHE SUSHITRACK_SANITIZE PROPERTY STRINGS "" address thread)

if(SUSHITRACK_SANITIZE STREQUAL "address")
    set(SUSHITRACK_SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer
        -fno-sanitize-recover=undefined)
elseif(SUSHITRACK_SANITIZE STREQUAL "thread")
    set(SUSHITRACK_SANITIZER_FLAGS -fsanitize=thread)
elseif(NOT SUSHITRACK_SANITIZE STREQUAL "")
    message(FATAL_ERROR
        "SUSHITRACK_SANITIZE is '${SUSHITRACK_SANITIZE}'; it must be empty, address or thread")
endif()

if(SUSHITRACK_SANITIZER_FLAGS AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR
        "SUSHITRACK_SANITIZE=${SUSHITRACK_SANITIZE} needs GCC or Clang; the compiler is "
        "${CMAKE_CXX_COMPILER_ID}")
endif()

# Compiles and links <target> with the sanitizer flags, when a sanitizer is set.
function(sushitrack_apply_sanitizers target)
    if(SUSHITRACK_SANITIZER_FLAGS)
        target_compile_options(${target} PRIVATE ${SUSHITRACK_SANITIZER_FLAGS})
        target_link_options(${target} PRIVATE ${SUSHITRACK_SANITIZER_FLAGS})
    endif()
endfunction()
