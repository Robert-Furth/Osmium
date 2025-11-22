cmake_minimum_required(VERSION 3.20)

project(Osmium VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(osmium)
add_subdirectory(gui)
