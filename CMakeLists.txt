# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

cmake_minimum_required(VERSION 3.14)
project(sushitrack VERSION 1.0.0 LANGUAGES CXX C)

# ============================================================
# Build Type & Options
# ============================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ------------------------------------------------------------
# Output Directories (Unified for Windows DLL/EXE resolution)
# ------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)



# ============================================================
# C++ Standard
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Shared Library Symbol Export
#
# Hidden by default, on every platform; only symbols carrying SUSHITRACK_API
# (include/SushiTrack/sushitrack_export.hpp) are exported. Windows needs no
# CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS auto-export: every function in the public C
# API (include/SushiTrack/sushitrack_c.h) already carries SUSHITRACK_API, so
# __declspec(dllexport)/dllimport does the same job the .hpp's
# __attribute__((visibility("default"))) branch does on GCC/Clang, and an
# internal symbol that forgot the macro stays hidden on both, instead of
# leaking out on Windows only.
# ============================================================
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# ============================================================
# Compiler Flags
# ============================================================
if(MSVC)
    add_compile_options(
        /W3          # Reasonable warnings
        /O2          # Optimize for speed
        /Ot          # Favor fast code
        /fp:fast     # Fast floating point
        /arch:AVX2   # Enable AVX2 instructions for Ryzen/Modern CPUs
        /DNDEBUG
        # C4251 ("needs to have dll-interface"): fires because Tracker
        # (include/SushiTrack/tracker.hpp) is SUSHITRACK_API for the internal
        # targets that link its C++ symbols directly (unit/integration tests,
        # tracker_bridge), and now surfaces now that hidden-by-default export
        # goes through real dllexport/dllimport instead of
        # CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS's bulk .def export. Safe to
        # silence here: every target in this build tree shares one compiler
        # and one CRT version, which is the exact condition C4251 exists to
        # flag as unsafe. The public C ABI (sushitrack_c.h) never exposes
        # Tracker, so no external consumer is affected either way.
        /wd4251
    )
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    # GCC / Clang (Linux & MinGW)
    add_compile_options(
        -Wall
        -O3
        -ffast-math
        -funroll-loops
        -DNDEBUG
    )
    # Mirrors the MSVC branch's /arch:AVX2 -- same x86 assumption, same ISA
    # target -- so a "fast" build behaves the same across compilers instead of
    # only opting into AVX2 on Windows. Skipped outright on a non-x86 host
    # (e.g. ARM), where -mavx2 would fail to compile rather than no-op.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i686)$")
        add_compile_options(-mavx2)
    endif()
endif()

# ============================================================
# Eigen
# ============================================================
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen")
if(EXISTS "${EIGEN3_INCLUDE_DIR}/Eigen/Dense")
    message(STATUS "Found local Eigen at: ${EIGEN3_INCLUDE_DIR}")
    if(NOT TARGET Eigen3::Eigen)
        add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
        set_target_properties(Eigen3::Eigen PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}"
        )
    endif()
else()
    message(WARNING "Eigen not found at ${EIGEN3_INCLUDE_DIR}. Build may fail.")
endif()

# ============================================================
# SushiTrack Library Target
# ============================================================
set(SUSHITRACK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/kalman_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/imm_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/tracklet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/sushitrack_c.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/config_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/distance_strategies.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/assignment_solvers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/lapjv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/rect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/logger.cpp
)

add_library(${PROJECT_NAME} SHARED ${SUSHITRACK_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE SUSHITRACK_EXPORTS)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)

# nlohmann/json is a private implementation detail of the JSON config loader;
# it must not leak onto the public include interface.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# ------------------------------------------------------------
# Internal object-library twin, for whitebox test linkage only.
#
# Hidden-by-default visibility (above) is correct for the public DLL
# boundary: only SUSHITRACK_API-tagged symbols (the C API, plus Tracker
# itself) should ever be resolvable from outside sushitrack.dll/.so. But
# tests/unit and tests/integration link directly against internal classes
# that were never meant to be part of that public surface (Rect<T>,
# LAPJVSolver, the cost calculators, Logger, ...) -- exporting all of those
# just to satisfy the test linker would defeat the point of hiding them.
# Compiling the same sources a second time sidesteps the question: with no
# DLL boundary, hidden visibility never stops the final test executable's
# linker from resolving any of its symbols. SUSHITRACK_STATIC (see
# include/SushiTrack/sushitrack_export.hpp) makes SUSHITRACK_API a no-op
# here, so this target's own translation units never see a dllexport
# attribute -- there is nothing to export, only ordinary external linkage
# within one link step.
#
# OBJECT, not STATIC: several classes (IMMFilter, NSAKalmanFilter, ...)
# register themselves into KalmanFilterRegistry purely as a static
# initializer's side effect (kalman_filter_registry.hpp's
# KalmanFilterRegistrar) -- nothing else in the program ever calls a
# function these translation units define, so nothing creates an
# unresolved symbol for the linker to pull their .obj in for. A STATIC
# library is an archive that a consuming linker extracts members from on
# demand; with no such demand, MSVC's linker silently drops
# imm_filter.obj entirely, the registrar's constructor never runs, and
# `st test --suite unit` on a kalman_type=2 (IMM) tracker crashes on a
# null IStatePredictor from the now-unregistered factory -- root-caused
# 2026-09-22 against a STATIC first attempt at this same target; kept here
# so the next person who reaches for STATIC hits this comment first. An
# OBJECT library carries no archive step:
# CMake hands every one of its .obj files to a consumer's link line
# unconditionally, the same way listing these sources directly on
# ${PROJECT_NAME} already does for the SHARED library above.
# ------------------------------------------------------------
add_library(${PROJECT_NAME}_static OBJECT EXCLUDE_FROM_ALL ${SUSHITRACK_SOURCES})
# PUBLIC, not PRIVATE: a consumer (unit_test, integration_test) including
# sushitrack_c.h must see SUSHITRACK_STATIC too, or SUSHITRACK_API resolves to
# its default __declspec(dllimport) branch in the consumer's own translation
# units -- an import thunk with nothing to import from, since this target has
# no DLL. Propagating the define is what makes "link this object library"
# behave like ordinary static linking on the consumer side as well.
target_compile_definitions(${PROJECT_NAME}_static PUBLIC SUSHITRACK_STATIC)
target_include_directories(${PROJECT_NAME}_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)
target_include_directories(${PROJECT_NAME}_static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)


# ============================================================
# External Trackers (third_party)
# ============================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/bytetrack
                 ${CMAKE_CURRENT_BINARY_DIR}/bytetrack)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/ocsort
                 ${CMAKE_CURRENT_BINARY_DIR}/ocsort)



# ============================================================
# Test Targets
#
# Unit tests (fast, no external data):
#   cmake -DBUILD_UNIT_TEST=ON ..
#   cmake --build . --target unit_test
#
# Regression tests (MOT label-based benchmark + video inference bridge):
#   cmake -DBUILD_REGRESSION_TEST=ON ..
#   cmake --build . --target regression_test
#
# Build both:
#   cmake -DBUILD_UNIT_TEST=ON -DBUILD_REGRESSION_TEST=ON ..
#
# Legacy alias (deprecated, maps to BUILD_REGRESSION_TEST):
#   cmake -DBUILD_MOT_TEST=ON ..
# ============================================================
option(BUILD_MOT_TEST "Deprecated alias for BUILD_REGRESSION_TEST" OFF)

# Testing must be enabled here, at the top level: enable_testing() only makes the
# CTestTestfile of the directory it runs in, so calling it solely inside
# tests/*/CMakeLists.txt left the root CTestTestfile empty and `ctest` from the
# build root reported "No tests were found". With it here, the root file recurses
# into the test subdirectories and `ds test` (ctest -L <label>) sees every test.
enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
