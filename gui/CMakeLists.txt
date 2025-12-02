cmake_minimum_required(VERSION 3.20)

project(OsmiumGui VERSION ${CMAKE_PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

if(WIN32)
    # Windows portable apps usually have the exe and dlls in the root, but
    # qt_standard_project_setup() includes GNUInstallDirs which sets it to
    # "bin" instead. So, override it here.
    set(CMAKE_INSTALL_BINDIR ".")
endif()

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Widgets Network)
qt_standard_project_setup()

set(app_icon_resource_windows "${PROJECT_SOURCE_DIR}/res.rc")
qt_add_executable(OsmiumGui
    ${app_icon_resource_windows}
    .gitignore
    src/controls/colorpicker.cpp
    src/controls/colorpicker.h
    src/controls/colorpicker.ui
    src/controls/labeledslider.cpp
    src/controls/labeledslider.h
    src/controls/pathchooser.cpp
    src/controls/pathchooser.h
    src/controls/previewer.cpp
    src/controls/previewer.h
    src/xmacro/h26x_preset.txt
    src/xmacro/video_codec.txt
    src/config.cpp
    src/config.h
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
    src/resources/icon-folder-open.png
    src/resources/icon-gear.png
    src/resources/icon-render.png
    src/resources/osmium.ico
    src/scoperenderer.cpp
    src/scoperenderer.h
    src/workers.cpp
    src/workers.h
)

target_link_libraries(OsmiumGui
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    OsmiumLib
)

target_include_directories(OsmiumGui
    PRIVATE "${PROJECT_SOURCE_DIR}/include"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
# if(${Qt6_VERSION} VERSION_LESS 6.1.0)
#   set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Osmium)
# endif()

set_target_properties(OsmiumGui PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME Osmium
)

install(TARGETS OsmiumGui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy over DLLs on install
if(WIN32)
    cmake_path(REPLACE_EXTENSION bass_lib ".dll" OUTPUT_VARIABLE bass_dll)
    cmake_path(REPLACE_EXTENSION bassmidi_lib ".dll" OUTPUT_VARIABLE bassmidi_dll)
    install(FILES ${bass_dll} ${bassmidi_dll} TYPE BIN)

    add_custom_command(TARGET OsmiumGui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${bass_dll} ${bassmidi_dll} $<TARGET_FILE_DIR:OsmiumGui>)
endif()

qt_generate_deploy_app_script(
    TARGET OsmiumGui
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
