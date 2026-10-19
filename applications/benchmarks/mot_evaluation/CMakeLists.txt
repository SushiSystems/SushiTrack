# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Builds the mot_evaluation benchmark and mot_evaluation_runner, the part its tests link.

add_library(mot_evaluation_runner STATIC mot_runner.cpp)
target_include_directories(mot_evaluation_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mot_evaluation_runner PUBLIC tracker_adapters)

add_executable(mot_evaluation main.cpp)
target_link_libraries(mot_evaluation PRIVATE mot_evaluation_runner)
