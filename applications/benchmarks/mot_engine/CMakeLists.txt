# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Builds the mot_engine benchmark and mot_engine_runner, the part its tests link.

add_library(mot_engine_runner STATIC mot_runner.cpp)
target_include_directories(mot_engine_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mot_engine_runner PUBLIC tracker_adapters)

add_executable(mot_engine main.cpp)
target_link_libraries(mot_engine PRIVATE mot_engine_runner)
