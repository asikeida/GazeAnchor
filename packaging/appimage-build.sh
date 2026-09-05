#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-/tmp/gaze-anchor-appimage-build}"
appdir="${APPDIR:-/tmp/GazeAnchor.AppDir}"
binary_name="gaze-anchor"
app_id="io.github.gazeanchor.GazeAnchor"
desktop_file="$appdir/usr/share/applications/$app_id.desktop"
metainfo_file="$appdir/usr/share/metainfo/$app_id.metainfo.xml"
icon_file="$appdir/usr/share/icons/hicolor/scalable/apps/$app_id.svg"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'missing required command: %s\n' "$1" >&2
    exit 1
  fi
}

optional_cmd() {
  command -v "$1" >/dev/null 2>&1
}

info() {
  printf '[info] %s\n' "$1"
}

require_path() {
  if [ ! -e "$1" ]; then
    printf 'missing required path: %s\n' "$1" >&2
    exit 1
  fi
}

require_cmd cmake
require_cmd ldd

info "Configuring FULL release build into $build_dir"

cmake -S "$repo_root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_PROFILE=FULL \
  -DCMAKE_INSTALL_PREFIX=/usr

info "Building GazeAnchor"
cmake --build "$build_dir" -j

info "Recreating AppDir at $appdir"
rm -rf "$appdir"
DESTDIR="$appdir" cmake --install "$build_dir"

require_path "$appdir/usr/bin/$binary_name"
require_path "$desktop_file"
require_path "$metainfo_file"
require_path "$icon_file"

cp "$desktop_file" "$appdir/"
cp "$icon_file" "$appdir/$app_id.svg"

cat > "$appdir/AppRun" <<'SH'
#!/usr/bin/env bash
set -e
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$HERE/usr/lib64:${LD_LIBRARY_PATH:-}"
export XDG_DATA_DIRS="$HERE/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
exec "$HERE/usr/bin/gaze-anchor" "$@"
SH
chmod +x "$appdir/AppRun"

if optional_cmd desktop-file-validate; then
  info "Validating desktop file"
  desktop-file-validate "$desktop_file"
fi

if optional_cmd appstreamcli; then
  info "Validating metainfo"
  appstreamcli validate "$metainfo_file"
fi

info "Recording direct runtime dependencies"
ldd "$appdir/usr/bin/$binary_name" | tee "$appdir/ldd.txt"

info "Smoke testing AppRun entrypoint"
"$appdir/AppRun" --version

if optional_cmd linuxdeployqt; then
  info "Running linuxdeployqt to bundle Qt and non-Qt runtime libraries"
  linuxdeployqt "$desktop_file" -bundle-non-qt-libs -unsupported-allow-new-glibc
elif optional_cmd linuxdeployqt6; then
  info "Running linuxdeployqt6 to bundle Qt and non-Qt runtime libraries"
  linuxdeployqt6 "$desktop_file" -bundle-non-qt-libs -unsupported-allow-new-glibc
else
  info "linuxdeployqt not found; AppDir currently depends on host Qt/KF6/LayerShellQt libraries"
fi

if [ "${CREATE_APPIMAGE:-0}" = "1" ]; then
  require_cmd appimagetool
  info "Creating AppImage"
  appimagetool "$appdir"
fi

cat <<'MSG'
AppDir prepared.

Recommended next steps:

1. Inspect direct dependencies recorded in AppDir/ldd.txt.
2. Install linuxdeployqt or linuxdeployqt6 and rerun this script to bundle Qt/KF6 libraries.
3. After bundling, create the AppImage with:

     CREATE_APPIMAGE=1 bash packaging/appimage-build.sh

Without linuxdeployqt, this AppDir is only a staging directory and still relies
on host KDE/Qt Wayland integration libraries.
MSG
