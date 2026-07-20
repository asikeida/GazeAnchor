cmake_minimum_required(VERSION 3.21)

project(MotionStabilizerLinux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(ENABLE_LAYER_SHELL_QT "Enable KDE/Wayland LayerShellQt overlay backend" ON)
option(ENABLE_KGLOBALACCEL "Enable KDE global shortcuts via KGlobalAccel" ON)
option(ENABLE_X11_FALLBACK "Enable X11/XFixes input passthrough fallback" ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)

if (ENABLE_LAYER_SHELL_QT)
    find_package(LayerShellQt QUIET)
endif()

if (ENABLE_KGLOBALACCEL)
    find_package(KF6GlobalAccel QUIET)
endif()

if (ENABLE_X11_FALLBACK)
    pkg_check_modules(X11_FALLBACK x11 xfixes)
endif()

qt_add_executable(motion-stabilizer-linux
    src/main.cpp
    src/AppConfig.cpp
    src/AppConfig.h
    src/OverlayWidget.cpp
    src/OverlayWidget.h
    src/SettingsWindow.cpp
    src/SettingsWindow.h
)

target_link_libraries(motion-stabilizer-linux
    PRIVATE
        Qt6::Widgets
)

target_compile_options(motion-stabilizer-linux PRIVATE -Wall -Wextra -Wpedantic)

if (X11_FALLBACK_FOUND)
    target_compile_definitions(motion-stabilizer-linux PRIVATE HAVE_X11_FALLBACK=1)
    target_include_directories(motion-stabilizer-linux PRIVATE ${X11_FALLBACK_INCLUDE_DIRS})
    target_link_libraries(motion-stabilizer-linux PRIVATE ${X11_FALLBACK_LIBRARIES})
endif()

if (LayerShellQt_FOUND)
    target_compile_definitions(motion-stabilizer-linux PRIVATE HAVE_LAYER_SHELL_QT=1)
    target_link_libraries(motion-stabilizer-linux PRIVATE LayerShellQt::Interface)
endif()

if (KF6GlobalAccel_FOUND)
    target_compile_definitions(motion-stabilizer-linux PRIVATE HAVE_KGLOBALACCEL=1)
    target_link_libraries(motion-stabilizer-linux PRIVATE KF6::GlobalAccel)
endif()

message(STATUS "LayerShellQt support: ${LayerShellQt_FOUND}")
message(STATUS "KGlobalAccel support: ${KF6GlobalAccel_FOUND}")
message(STATUS "X11 fallback support: ${X11_FALLBACK_FOUND}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/motion-stabilizer-linux.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/motion-stabilizer-linux.desktop
    @ONLY
)

include(GNUInstallDirs)
install(TARGETS motion-stabilizer-linux RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/motion-stabilizer-linux.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
