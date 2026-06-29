cmake_minimum_required(VERSION 3.14)
project(sushitrack)

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
# Windows (MSVC): auto-generate .lib import library
# Linux (GCC/Clang): default visibility hidden, export explicitly
# ============================================================
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(NOT WIN32)
    set(CMAKE_CXX_VISIBILITY_PRESET default)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN OFF)
endif()

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
endif()

# ============================================================
# Eigen
# ============================================================
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/eigen")
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
add_library(${PROJECT_NAME} SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KalmanFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IMMFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tracklet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SushiTrackTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SushiTrackC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DistanceStrategies.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AssignmentSolvers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAPJV.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Rect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logger.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)

# nlohmann/json is a private implementation detail of the JSON config loader;
# it must not leak onto the public include interface.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)


# ============================================================
# External Trackers (3rdparty)
# ============================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bytetrack
                 ${CMAKE_CURRENT_BINARY_DIR}/bytetrack)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ocsort
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

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
