# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.

# Builds the tracker bridge and the MOT benchmark.

add_subdirectory(tracker_bridge)
add_subdirectory(benchmarks/mot_engine)
