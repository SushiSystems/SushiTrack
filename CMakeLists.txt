cmake_minimum_required(VERSION 3.14)
project(sushitrack)

# ============================================================
# Build Type & Options
# ============================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(USE_ASAN "Enable AddressSanitizer" OFF)
if(USE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()


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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tracklet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SushiTrackTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SushiTrackC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DistanceStrategies.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AssignmentSolvers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LAPJV.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TrackFSM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InteractionGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Rect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logger.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)

# ============================================================
# External Trackers (3rdparty)
# ============================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bytetrack
                 ${CMAKE_CURRENT_BINARY_DIR}/bytetrack)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ocsort
                 ${CMAKE_CURRENT_BINARY_DIR}/ocsort)


# ============================================================
# MOT Test Executable
# BUILD_MOT_TEST=ON to enable (default: OFF)
# ============================================================
option(BUILD_MOT_TEST "Build the unified MOT benchmark test executable" OFF)

if(BUILD_MOT_TEST)
    find_package(GTest REQUIRED)

    add_executable(mot_test
        ${CMAKE_CURRENT_SOURCE_DIR}/test/cpp/MainTest.cpp
    )

    target_include_directories(mot_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test/cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bytetrack/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ocsort/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
        ${EIGEN3_INCLUDE_DIR}
    )

    target_link_libraries(mot_test PRIVATE
        ${PROJECT_NAME}   # sushitrack shared lib
        bytetrack         # bytetrack static lib
        libocsort         # ocsort shared lib
        GTest::GTest
    )

    # Tracker Bridge for Python ctypes
    add_library(tracker_bridge SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/test/cpp/TrackerRegistry.cpp
    )
    target_include_directories(tracker_bridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test/cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/bytetrack/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ocsort/include
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
        ${EIGEN3_INCLUDE_DIR}
    )
    target_link_libraries(tracker_bridge PRIVATE
        ${PROJECT_NAME}
        bytetrack
        libocsort
    )

    # CTest integration
    enable_testing()
    include(GoogleTest)
    gtest_discover_tests(mot_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
