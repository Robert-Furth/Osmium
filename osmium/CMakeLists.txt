cmake_minimum_required(VERSION 3.18)

project(OsmiumLib VERSION ${CMAKE_PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(local_lib_path "${PROJECT_SOURCE_DIR}/libs/${CMAKE_SYSTEM_PROCESSOR}")

add_library(OsmiumLib STATIC
    src/error.cpp
    src/error.h
    src/eventtracker.cpp
    src/eventtracker.h
    src/handlewrapper.cpp
    src/handlewrapper.h
    src/osmium.cpp
    src/osmium.h
    src/player.cpp
    src/player.h
    src/scope.cpp
    src/scope.h
    src/scopebuilder.cpp
    src/scopebuilder.h
)

target_include_directories(OsmiumLib
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    INTERFACE ${PROJECT_SOURCE_DIR}/src
)

find_library(bass_lib
    NAMES bass
    PATHS ${local_lib_path}
)

find_library(bassmidi_lib
    NAMES bassmidi
    PATHS ${local_lib_path}
)

target_link_libraries(OsmiumLib
    PUBLIC ${bass_lib} ${bassmidi_lib}
)
