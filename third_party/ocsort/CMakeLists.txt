cmake_minimum_required(VERSION 3.10)
project(libocsort)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SRC_LIST src/*.cpp)

# Using STATIC to avoid DLL issues in test builds if not needed as shared
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} Eigen3::Eigen)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4244 /wd4267)
endif()
