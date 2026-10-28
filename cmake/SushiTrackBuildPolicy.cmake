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

# Every build type takes the release C runtime, so Debug links the Release packages beneath it
# (SushiLog, SushiProfiler) on one heap; ignored off the MSVC ABI. Memory errors are the
# sanitizer tree's job.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# A Debug consumer of an imported target links its Release build, which shares that CRT.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release RelWithDebInfo MinSizeRel "")

# The MSVC STL's container layout depends on this; libstdc++ and libc++ never read it.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:_ITERATOR_DEBUG_LEVEL=0>")
endif()

# Debug host code keeps line tables only: stack traces get files and lines at a fraction of full
# debug information's compile memory, object size and link time. The later flag overrides -g.
if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    add_compile_options("$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:-gline-tables-only>")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Optimisation and the assert switch belong to every build type but Debug: Debug is the build to
# step through, with the asserts on. The floating-point model is the same in all of them.
if(MSVC)
    add_compile_options(/W3 /fp:fast /arch:AVX2 /wd4251)
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/O2;/Ot;/DNDEBUG>")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    # NaN and Inf keep their meaning, as under /fp:fast; the tracker refuses non-finite input.
    add_compile_options(-Wall -ffast-math -fno-finite-math-only)
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:-O3;-funroll-loops;-DNDEBUG>")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i686)$")
        # Eigen emits FMA whenever AVX2 is on under an MSVC-compatible compiler, as /arch:AVX2 allows.
        add_compile_options(-mavx2 -mfma)
    endif()
endif()
