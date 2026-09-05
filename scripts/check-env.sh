#!/usr/bin/env bash
set -u

fail=0

first_existing_path=''

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

find_first_existing() {
    first_existing_path=''
    for path in "$@"; do
        if [ -e "$path" ]; then
            first_existing_path="$path"
            return 0
        fi
    done
    return 1
}

check_optional_paths() {
    label="$1"
    shift
    if find_first_existing "$@"; then
        printf '[ok] optional: %s %s\n' "$label" "$first_existing_path"
    else
        printf '[optional-missing] %s\n' "$label"
    fi
}

check_optional_cmd() {
    if command -v "$1" >/dev/null 2>&1; then
        printf '[ok] optional command: %s\n' "$1"
    else
        printf '[optional-missing] command: %s\n' "$1"
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
check_cmd appstreamcli
check_cmd desktop-file-validate
check_optional_cmd dpkg-buildpackage
check_optional_cmd rpmbuild
check_optional_cmd flatpak-builder
check_optional_cmd linuxdeployqt

printf '\nQt pkg-config modules\n'
check_pkg_config Qt6Core
check_pkg_config Qt6Gui
check_pkg_config Qt6Widgets
check_pkg_config x11
check_pkg_config xfixes

printf '\nKDE CMake packages\n'
check_optional_paths \
  'LayerShellQt CMake package: KDE Wayland overlay support will be degraded without it.' \
  /usr/lib/cmake/LayerShellQt/LayerShellQtConfig.cmake \
  /usr/lib64/cmake/LayerShellQt/LayerShellQtConfig.cmake \
  /usr/lib/x86_64-linux-gnu/cmake/LayerShellQt/LayerShellQtConfig.cmake
check_optional_paths \
  'KF6GlobalAccel CMake package: global shortcuts will be unavailable without it.' \
  /usr/lib/cmake/KF6GlobalAccel/KF6GlobalAccelConfig.cmake \
  /usr/lib64/cmake/KF6GlobalAccel/KF6GlobalAccelConfig.cmake \
  /usr/lib/x86_64-linux-gnu/cmake/KF6GlobalAccel/KF6GlobalAccelConfig.cmake

printf '\nRuntime pieces\n'
check_optional_paths \
  'layer-shell Wayland plugin' \
  /usr/lib/qt6/plugins/wayland-shell-integration/liblayer-shell.so \
  /usr/lib64/qt6/plugins/wayland-shell-integration/liblayer-shell.so \
  /usr/lib/x86_64-linux-gnu/qt6/plugins/wayland-shell-integration/liblayer-shell.so
check_optional_paths \
  'LayerShellQt runtime library' \
  /usr/lib/libLayerShellQtInterface.so \
  /usr/lib/libLayerShellQtInterface.so.6 \
  /usr/lib64/libLayerShellQtInterface.so \
  /usr/lib64/libLayerShellQtInterface.so.6 \
  /usr/lib/x86_64-linux-gnu/libLayerShellQtInterface.so \
  /usr/lib/x86_64-linux-gnu/libLayerShellQtInterface.so.6
check_optional_paths \
  'KF6GlobalAccel runtime library' \
  /usr/lib/libKF6GlobalAccel.so \
  /usr/lib/libKF6GlobalAccel.so.6 \
  /usr/lib64/libKF6GlobalAccel.so \
  /usr/lib64/libKF6GlobalAccel.so.6 \
  /usr/lib/x86_64-linux-gnu/libKF6GlobalAccel.so \
  /usr/lib/x86_64-linux-gnu/libKF6GlobalAccel.so.6

printf '\nProject assets\n'
check_path packaging/io.github.gazeanchor.GazeAnchor.svg
check_path packaging/io.github.gazeanchor.GazeAnchor.metainfo.xml
check_path packaging/io.github.gazeanchor.GazeAnchor.yml
check_path packaging/gaze-anchor.spec
check_path packaging/PKGBUILD
check_path packaging/.SRCINFO
check_path packaging/build-deb-package.sh
check_path packaging/build-rpm-package.sh
check_path packaging/build-source-tarball.sh
check_path packaging/collect-release-artifacts.sh
check_path packaging/generate-release-notes.sh
check_path packaging/prepare-release-version.sh
check_path packaging/verify-release-consistency.sh
check_path packaging/verify-deb-package.sh
check_path packaging/verify-rpm-package.sh
check_path packaging/version.sh
check_path debian/control
check_path debian/rules
check_path debian/changelog
check_path debian/gaze-anchor.install
check_path debian/source/format

printf '\nCompatibility note\n'
printf '  Best supported: KDE Plasma Wayland with LayerShellQt and KGlobalAccel.\n'
printf '  Generic X11: supported when built with x11+xfixes.\n'
printf '  Generic Wayland: may run, but overlay/topmost/global-shortcut behavior depends on compositor policy.\n'
printf '  CLI fallback: use gaze-anchor action show-settings|toggle-overlay|toggle-crosshair|toggle-clock|quit.\n'

exit "$fail"
