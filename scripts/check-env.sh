#!/usr/bin/env bash
set -u

fail=0

check_cmd() {
    if command -v "$1" >/dev/null 2>&1; then
        printf '[ok] command: %s\n' "$1"
    else
        printf '[missing] command: %s\n' "$1"
        fail=1
    fi
}

check_pkg_config() {
    if pkg-config --exists "$1"; then
        printf '[ok] pkg-config: %s %s\n' "$1" "$(pkg-config --modversion "$1")"
    else
        printf '[missing] pkg-config: %s\n' "$1"
        fail=1
    fi
}

check_path() {
    if [ -e "$1" ]; then
        printf '[ok] path: %s\n' "$1"
    else
        printf '[missing] path: %s\n' "$1"
        fail=1
    fi
}

printf 'Session\n'
printf '  XDG_SESSION_TYPE=%s\n' "${XDG_SESSION_TYPE:-}"
printf '  XDG_CURRENT_DESKTOP=%s\n' "${XDG_CURRENT_DESKTOP:-}"
printf '  WAYLAND_DISPLAY=%s\n' "${WAYLAND_DISPLAY:-}"
printf '  DISPLAY=%s\n' "${DISPLAY:-}"

if [ "${XDG_SESSION_TYPE:-}" != "wayland" ]; then
    printf '[warn] This build is currently optimized for KDE Wayland.\n'
fi

if [ "${XDG_CURRENT_DESKTOP:-}" != "KDE" ]; then
    printf '[warn] This build is currently optimized for KDE Plasma.\n'
fi

printf '\nCommands\n'
check_cmd cmake
check_cmd qmake6
check_cmd pkg-config

printf '\nQt pkg-config modules\n'
check_pkg_config Qt6Core
check_pkg_config Qt6Gui
check_pkg_config Qt6Widgets

printf '\nKDE CMake packages\n'
if [ -e /usr/lib/cmake/LayerShellQt/LayerShellQtConfig.cmake ]; then
    printf '[ok] optional KDE package: LayerShellQt\n'
else
    printf '[optional-missing] LayerShellQt: KDE Wayland overlay support will be degraded.\n'
fi
if [ -e /usr/lib/cmake/KF6GlobalAccel/KF6GlobalAccelConfig.cmake ]; then
    printf '[ok] optional KDE package: KGlobalAccel\n'
else
    printf '[optional-missing] KGlobalAccel: F1/F2/F3 global shortcuts will be unavailable.\n'
fi

printf '\nRuntime pieces\n'
if [ -e /usr/lib/qt6/plugins/wayland-shell-integration/liblayer-shell.so ]; then
    printf '[ok] optional runtime: layer-shell Wayland plugin\n'
else
    printf '[optional-missing] layer-shell Wayland plugin\n'
fi
if [ -e /usr/lib/libLayerShellQtInterface.so ]; then
    printf '[ok] optional runtime: LayerShellQt library\n'
else
    printf '[optional-missing] LayerShellQt library\n'
fi
if [ -e /usr/lib/libKF6GlobalAccel.so ]; then
    printf '[ok] optional runtime: KGlobalAccel library\n'
else
    printf '[optional-missing] KGlobalAccel library\n'
fi

printf '\nCompatibility note\n'
printf '  Best supported: KDE Plasma Wayland with LayerShellQt and KGlobalAccel.\n'
printf '  Generic X11: supported when built with x11+xfixes.\n'
printf '  Generic Wayland: may run, but overlay/topmost/global-shortcut behavior depends on compositor policy.\n'

exit "$fail"
