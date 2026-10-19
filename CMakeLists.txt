# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

cmake_minimum_required(VERSION 3.22)
project(sushitrack VERSION 1.0.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(SushiTrackBuildPolicy)
include(SushiTrackEigen)
include(SushiTrackSushiLog)
include(SushiTrackTests)

option(SUSHITRACK_BUILD_TESTS "Build sushitrack_tests from the mirror under tests/" ON)
option(SUSHITRACK_BUILD_APPLICATIONS "Build tracker_bridge and the mot_engine benchmark" ON)

add_subdirectory(source)
if(SUSHITRACK_BUILD_APPLICATIONS)
    add_subdirectory(applications)
endif()

enable_testing()
add_subdirectory(tests)
