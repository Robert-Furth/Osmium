cmake_minimum_required(VERSION 3.16)

project(OsmiumLib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
    error.h
    eventtracker.cpp
    eventtracker.h
    handlewrapper.cpp
    handlewrapper.h
    osmium.cpp
    osmium.h
    player.cpp
    player.h
    scope.cpp
    scope.h
    scopebuilder.cpp
    scopebuilder.h
)

add_library(OsmiumLib STATIC ${PROJECT_SOURCES})

target_include_directories(OsmiumLib
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_directories(OsmiumLib
    PUBLIC ${PROJECT_SOURCE_DIR}/libs
)

target_link_libraries(OsmiumLib
    PUBLIC bass
    PUBLIC bassmidi
)

# Copy relevant DLLs to the build dir on Windows
# if(WIN32)
#     add_custom_command(
#         TARGET OsmiumLib POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${CMAKE_SOURCE_DIR}/libs/bass24-x64/bass.dll"
#             $<TARGET_FILE_DIR:OsmiumLib>
#     )
# endif()
