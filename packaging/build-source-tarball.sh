#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_root="${DIST_ROOT:-$repo_root/dist/release}"
version="$(bash "$repo_root/packaging/version.sh" release-version)"
staging_root="${SOURCE_STAGING_ROOT:-/tmp/opencode/gaze-anchor-source}"
staging_dir="$staging_root/GazeAnchor-$version"
tarball_path="$dist_root/source/gaze-anchor-$version.tar.gz"

info() {
  printf '[info] %s\n' "$1"
}

mkdir -p "$dist_root/source"
rm -rf "$staging_root"
mkdir -p "$staging_dir"

info "Staging source tree into $staging_dir"
cp -a "$repo_root/." "$staging_dir/"

rm -rf \
  "$staging_dir/.git" \
  "$staging_dir/.github" \
  "$staging_dir/.qt" \
  "$staging_dir/build" \
  "$staging_dir/CMakeCache.txt" \
  "$staging_dir/CMakeFiles" \
  "$staging_dir/cmake_install.cmake" \
  "$staging_dir/Makefile" \
  "$staging_dir/gaze-anchor" \
  "$staging_dir/motion-stabilizer-linux" \
  "$staging_dir/motion-stabilizer-linux.desktop" \
  "$staging_dir/motion-stabilizer-linux_autogen" \
  "$staging_dir/dist" \
  "$staging_dir/opcode-summary"

for path in "$staging_dir"/cmake-build-*; do
  if [ -e "$path" ]; then
    rm -rf "$path"
  fi
done

info "Creating source tarball $tarball_path"
tar -czf "$tarball_path" -C "$staging_root" "GazeAnchor-$version"

printf '%s\n' "$tarball_path"
