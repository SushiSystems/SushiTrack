# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Builds the shared library sushitrack and its object twin sushitrack_static, which the tests and
# the applications link; docs/architecture/BUILD_SYSTEM.md gives the reasons for the twin.

file(GLOB_RECURSE SUSHITRACK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(sushitrack SHARED ${SUSHITRACK_SOURCES})
target_compile_definitions(sushitrack PRIVATE SUSHITRACK_EXPORTS)
target_include_directories(sushitrack PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR})
target_include_directories(sushitrack PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party)
target_link_libraries(sushitrack PRIVATE SushiLog::SushiLog)
sushitrack_apply_logging(sushitrack PRIVATE)
sushitrack_apply_profiling(sushitrack)
sushitrack_apply_sanitizers(sushitrack)

add_library(sushitrack_static OBJECT EXCLUDE_FROM_ALL ${SUSHITRACK_SOURCES})
target_compile_definitions(sushitrack_static PUBLIC SUSHITRACK_STATIC)
target_include_directories(sushitrack_static PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR})
target_include_directories(sushitrack_static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party)
target_link_libraries(sushitrack_static PUBLIC SushiLog::SushiLog)
sushitrack_apply_logging(sushitrack_static PUBLIC)
sushitrack_apply_profiling(sushitrack_static)
sushitrack_apply_sanitizers(sushitrack_static)
