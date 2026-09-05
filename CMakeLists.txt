cmake_minimum_required(VERSION 3.21)

project(GazeAnchor VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(APP_BINARY_NAME "gaze-anchor")
set(APP_DISPLAY_NAME "GazeAnchor")
set(APP_ORGANIZATION_NAME "GazeAnchor")
set(APP_ID "io.github.gazeanchor.GazeAnchor" CACHE STRING "Desktop and package application ID")
set(APP_DEVELOPER_ID "io.github.gazeanchor")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(BUILD_PROFILE "FULL" CACHE STRING "Feature profile: FULL, X11, or MINIMAL")
set_property(CACHE BUILD_PROFILE PROPERTY STRINGS FULL X11 MINIMAL)

if (NOT BUILD_PROFILE STREQUAL "FULL" AND NOT BUILD_PROFILE STREQUAL "X11" AND NOT BUILD_PROFILE STREQUAL "MINIMAL")
    message(FATAL_ERROR "Unsupported BUILD_PROFILE='${BUILD_PROFILE}'. Expected FULL, X11, or MINIMAL.")
endif()

option(ENABLE_LAYER_SHELL_QT "Enable KDE/Wayland LayerShellQt overlay backend" ON)
option(ENABLE_KGLOBALACCEL "Enable KDE global shortcuts via KGlobalAccel" ON)
option(ENABLE_X11_FALLBACK "Enable X11/XFixes input passthrough fallback" ON)

if (BUILD_PROFILE STREQUAL "FULL")
    set(ENABLE_LAYER_SHELL_QT ON CACHE BOOL "Enable KDE/Wayland LayerShellQt overlay backend" FORCE)
    set(ENABLE_KGLOBALACCEL ON CACHE BOOL "Enable KDE global shortcuts via KGlobalAccel" FORCE)
    set(ENABLE_X11_FALLBACK ON CACHE BOOL "Enable X11/XFixes input passthrough fallback" FORCE)
elseif (BUILD_PROFILE STREQUAL "X11")
    set(ENABLE_LAYER_SHELL_QT OFF CACHE BOOL "Enable KDE/Wayland LayerShellQt overlay backend" FORCE)
    set(ENABLE_KGLOBALACCEL OFF CACHE BOOL "Enable KDE global shortcuts via KGlobalAccel" FORCE)
    set(ENABLE_X11_FALLBACK ON CACHE BOOL "Enable X11/XFixes input passthrough fallback" FORCE)
elseif (BUILD_PROFILE STREQUAL "MINIMAL")
    set(ENABLE_LAYER_SHELL_QT OFF CACHE BOOL "Enable KDE/Wayland LayerShellQt overlay backend" FORCE)
    set(ENABLE_KGLOBALACCEL OFF CACHE BOOL "Enable KDE global shortcuts via KGlobalAccel" FORCE)
    set(ENABLE_X11_FALLBACK OFF CACHE BOOL "Enable X11/XFixes input passthrough fallback" FORCE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

if (BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

if (ENABLE_X11_FALLBACK)
    find_package(PkgConfig REQUIRED)
endif()

if (ENABLE_LAYER_SHELL_QT)
    find_package(LayerShellQt QUIET)
endif()

if (ENABLE_KGLOBALACCEL)
    find_package(KF6GlobalAccel QUIET)
endif()

if (ENABLE_X11_FALLBACK)
    pkg_check_modules(X11_FALLBACK x11 xfixes)
endif()

if (BUILD_PROFILE STREQUAL "FULL")
    if (NOT LayerShellQt_FOUND)
        message(FATAL_ERROR "BUILD_PROFILE=FULL requires LayerShellQt.")
    endif()
    if (NOT KF6GlobalAccel_FOUND)
        message(FATAL_ERROR "BUILD_PROFILE=FULL requires KF6GlobalAccel.")
    endif()
    if (NOT X11_FALLBACK_FOUND)
        message(FATAL_ERROR "BUILD_PROFILE=FULL requires x11 and xfixes via pkg-config.")
    endif()
elseif (BUILD_PROFILE STREQUAL "X11" AND NOT X11_FALLBACK_FOUND)
    message(FATAL_ERROR "BUILD_PROFILE=X11 requires x11 and xfixes via pkg-config.")
endif()

find_package(Git QUIET)
set(GIT_COMMIT "unknown")
if (Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BuildInfo.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/BuildInfo.h
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/gaze-anchor.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/${APP_ID}.desktop
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/io.github.gazeanchor.GazeAnchor.metainfo.xml
    ${CMAKE_CURRENT_BINARY_DIR}/generated/${APP_ID}.metainfo.xml
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/single-instance-smoke.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/single-instance-smoke.sh
    @ONLY
)

qt_add_executable(gaze_anchor
    src/main.cpp
    src/AppConfig.cpp
    src/AppConfig.h
    src/OverlayWidget.cpp
    src/OverlayWidget.h
    src/SettingsWindow.cpp
    src/SettingsWindow.h
)

set_target_properties(gaze_anchor PROPERTIES OUTPUT_NAME ${APP_BINARY_NAME})

target_link_libraries(gaze_anchor
    PRIVATE
        Qt6::Widgets
)

target_include_directories(gaze_anchor PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

target_compile_options(gaze_anchor PRIVATE -Wall -Wextra -Wpedantic)

if (X11_FALLBACK_FOUND)
    target_compile_definitions(gaze_anchor PRIVATE HAVE_X11_FALLBACK=1)
    target_include_directories(gaze_anchor PRIVATE ${X11_FALLBACK_INCLUDE_DIRS})
    target_link_libraries(gaze_anchor PRIVATE ${X11_FALLBACK_LIBRARIES})
endif()

if (LayerShellQt_FOUND)
    target_compile_definitions(gaze_anchor PRIVATE HAVE_LAYER_SHELL_QT=1)
    target_link_libraries(gaze_anchor PRIVATE LayerShellQt::Interface)
endif()

if (KF6GlobalAccel_FOUND)
    target_compile_definitions(gaze_anchor PRIVATE HAVE_KGLOBALACCEL=1)
    target_link_libraries(gaze_anchor PRIVATE KF6::GlobalAccel)
endif()

message(STATUS "LayerShellQt support: ${LayerShellQt_FOUND}")
message(STATUS "KGlobalAccel support: ${KF6GlobalAccel_FOUND}")
message(STATUS "X11 fallback support: ${X11_FALLBACK_FOUND}")
message(STATUS "Build profile: ${BUILD_PROFILE}")
message(STATUS "Git commit: ${GIT_COMMIT}")

include(GNUInstallDirs)
install(TARGETS gaze_anchor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/${APP_ID}.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/${APP_ID}.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/io.github.gazeanchor.GazeAnchor.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME ${APP_ID}.svg
)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${APP_BINARY_NAME}
)

if (BUILD_TESTING)
    qt_add_executable(appconfig-tests
        tests/AppConfigTest.cpp
        src/AppConfig.cpp
        src/AppConfig.h
    )
    target_include_directories(appconfig-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)
    target_link_libraries(appconfig-tests PRIVATE Qt6::Test Qt6::Core Qt6::Gui)
    add_test(NAME appconfig-tests COMMAND appconfig-tests)
    add_test(NAME single-instance-smoke COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/generated/single-instance-smoke.sh)
endif()
