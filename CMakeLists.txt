cmake_minimum_required(VERSION 3.18)

project(Osmium VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Widgets Network)
qt_standard_project_setup()

add_subdirectory(osmium)
# add_subdirectory(cli)

set(PROJECT_SOURCES
    src/controls/colorpicker.cpp
    src/controls/colorpicker.h
    src/controls/colorpicker.ui
    src/controls/labeledslider.cpp
    src/controls/labeledslider.h
    src/controls/pathchooser.cpp
    src/controls/pathchooser.h
    src/controls/previewer.cpp
    src/controls/previewer.h
    src/instrumentnames.cpp
    src/instrumentnames.h
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/optionsdialog.cpp
    src/optionsdialog.h
    src/optionsdialog.ui
    src/renderargs.h
    src/resources.qrc
    src/saveload.cpp
    src/saveload.h
    src/scoperenderer.cpp
    src/scoperenderer.h
    src/workers.cpp
    src/workers.h
)

qt_add_executable(Osmium ${PROJECT_SOURCES})

target_link_libraries(Osmium
    PRIVATE Qt6::Core
    PRIVATE Qt6::Widgets
    PRIVATE Qt6::Concurrent
    PRIVATE Qt6::Network
    PRIVATE OsmiumLib
)

target_include_directories(Osmium
    PRIVATE "${PROJECT_SOURCE_DIR}/include"
    PRIVATE "${PROJECT_SOURCE_DIR}/osmium"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
# if(${Qt6_VERSION} VERSION_LESS 6.1.0)
#   set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Osmium)
# endif()

set_target_properties(Osmium PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS Osmium OsmiumLib
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET Osmium
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
