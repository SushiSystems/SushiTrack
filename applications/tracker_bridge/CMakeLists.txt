# CMakeLists.txt
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# Builds tracker_adapters, the header-only adapters, and tracker_bridge, their C library.

add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/bytetrack ${PROJECT_BINARY_DIR}/bytetrack)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/ocsort ${PROJECT_BINARY_DIR}/ocsort)
# Both static libraries end up inside the shared tracker_bridge, which needs PIC on Linux.
set_target_properties(bytetrack libocsort PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(tracker_adapters INTERFACE)
target_include_directories(tracker_adapters INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party
    ${PROJECT_SOURCE_DIR}/third_party/bytetrack/include
    ${PROJECT_SOURCE_DIR}/third_party/ocsort/include)
target_link_libraries(tracker_adapters INTERFACE sushitrack bytetrack libocsort)

add_library(tracker_bridge SHARED tracker_bridge.cpp)
target_link_libraries(tracker_bridge PRIVATE tracker_adapters)
