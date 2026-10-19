# SushiTrackEigen.cmake
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Declares Eigen3::Eigen from the third_party/eigen submodule and sets EIGEN3_INCLUDE_DIR.

set(EIGEN3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/eigen")
if(EXISTS "${EIGEN3_INCLUDE_DIR}/Eigen/Dense")
    message(STATUS "Found local Eigen at: ${EIGEN3_INCLUDE_DIR}")
    if(NOT TARGET Eigen3::Eigen)
        add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
        set_target_properties(Eigen3::Eigen PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}")
    endif()
else()
    message(WARNING "Eigen not found at ${EIGEN3_INCLUDE_DIR}. Build may fail.")
endif()
