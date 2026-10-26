# SushiTrackBuildPolicy.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Sets the language standard, output folders, symbol visibility and compiler flags of every
# target in the tree; docs/architecture/BUILD_SYSTEM.md gives the reasons.

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(MSVC)
    add_compile_options(/W3 /O2 /Ot /fp:fast /arch:AVX2 /DNDEBUG /wd4251)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -O3 -ffast-math -funroll-loops -DNDEBUG)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i686)$")
        # Eigen emits FMA whenever AVX2 is on under an MSVC-compatible compiler, as /arch:AVX2 allows.
        add_compile_options(-mavx2 -mfma)
    endif()
endif()
