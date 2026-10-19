# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

cmake_minimum_required(VERSION 3.14)
project(sushitrack VERSION 1.0.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(SushiTrackBuildPolicy)
include(SushiTrackEigen)

option(BUILD_MOT_TEST "Deprecated alias for BUILD_REGRESSION_TEST" OFF)

add_subdirectory(source)
add_subdirectory(third_party/bytetrack ${PROJECT_BINARY_DIR}/bytetrack)
add_subdirectory(third_party/ocsort ${PROJECT_BINARY_DIR}/ocsort)

enable_testing()
add_subdirectory(tests)
