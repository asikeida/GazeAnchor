#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_root="${DIST_ROOT:-$repo_root/dist/release}"
repo_parent="$(dirname "$repo_root")"
rpm_topdir="${RPM_TOPDIR:-/tmp/opencode/gaze-anchor-rpmbuild}"
appdir_root="${APPDIR:-/tmp/GazeAnchor.AppDir}"
release_prefix="$(bash "$repo_root/packaging/version.sh" release-prefix)"

info() {
  printf '[info] %s\n' "$1"
}

copy_matches() {
  local target_dir="$1"
  shift
  mkdir -p "$target_dir"
  local copied=0
  for pattern in "$@"; do
    for path in $pattern; do
      if [ -e "$path" ]; then
        cp -f "$path" "$target_dir/"
        copied=1
      fi
    done
  done
  if [ "$copied" -eq 1 ]; then
    return 0
  fi
  return 1
}

mkdir -p "$dist_root/deb" "$dist_root/rpm" "$dist_root/appimage" "$dist_root/source"

info "Collecting Debian artifacts"
copy_matches "$dist_root/deb" \
  "$repo_parent"/gaze-anchor_*.deb \
  "$repo_parent"/gaze-anchor_*.changes \
  "$repo_parent"/gaze-anchor_*.buildinfo || true

info "Collecting RPM artifacts"
copy_matches "$dist_root/rpm" \
  "$rpm_topdir"/RPMS/*/*.rpm \
  "$rpm_topdir"/SRPMS/*.src.rpm || true

info "Collecting AppImage artifacts"
copy_matches "$dist_root/appimage" \
  "$repo_root"/*.AppImage || true

if [ -d "$appdir_root" ]; then
  tar -czf "$dist_root/appimage/${release_prefix}.AppDir.tar.gz" -C "$(dirname "$appdir_root")" "$(basename "$appdir_root")"
fi

if ! compgen -G "$dist_root/source/*.tar.gz" >/dev/null; then
  bash "$repo_root/packaging/build-source-tarball.sh" >/dev/null
fi

bash "$repo_root/packaging/generate-release-notes.sh" >/dev/null

checksum_file="$dist_root/SHA256SUMS"
info "Writing checksums to $checksum_file"
rm -f "$checksum_file"
(
  cd "$dist_root"
  find . -maxdepth 2 -type f ! -name 'SHA256SUMS' -print0 | sort -z | xargs -0 sha256sum
) > "$checksum_file"

printf '%s\n' "$dist_root"
