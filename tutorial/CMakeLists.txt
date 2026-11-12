# Preferable to avoid warnings during execution
cmake_minimum_required(VERSION 3.15)

project(TutorialSat VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(AriadneSat REQUIRED)

add_executable(tutorial_sat tutorial_sat.cpp)
target_link_libraries(tutorial_sat PRIVATE AriadneSat::ariadne-sat)
