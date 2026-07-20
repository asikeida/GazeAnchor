#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-/tmp/motion-stabilizer-linux-appimage-build}"
appdir="${APPDIR:-/tmp/MotionStabilizerLinux.AppDir}"

cmake -S "$repo_root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$build_dir" -j

rm -rf "$appdir"
DESTDIR="$appdir" cmake --install "$build_dir"

mkdir -p "$appdir/usr/share/metainfo"
cat > "$appdir/usr/share/metainfo/io.github.motionstabilizer.MotionStabilizer.appdata.xml" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>io.github.motionstabilizer.MotionStabilizer</id>
  <name>Motion Stabilizer Linux</name>
  <summary>Visual stabilization overlay for KDE Wayland</summary>
  <metadata_license>MIT</metadata_license>
  <project_license>MIT</project_license>
</component>
XML

cat > "$appdir/AppRun" <<'SH'
#!/usr/bin/env bash
set -e
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
exec "$HERE/usr/bin/motion-stabilizer-linux" "$@"
SH
chmod +x "$appdir/AppRun"

cp "$appdir/usr/share/applications/motion-stabilizer-linux.desktop" "$appdir/"

cat <<'MSG'
AppDir prepared.

To create an AppImage, install appimagetool and run:

  appimagetool /tmp/MotionStabilizerLinux.AppDir

Note: this AppDir relies on host KDE/Qt Wayland integration libraries unless
you add linuxdeployqt/linuxdeploy into the packaging pipeline.
MSG
