#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
topdir="${RPM_TOPDIR:-/tmp/opencode/gaze-anchor-rpmbuild}"
workdir="${RPM_WORKDIR:-/tmp/opencode/gaze-anchor-rpm-src}"
version="${RPM_VERSION:-$(bash "$repo_root/packaging/version.sh" release-version)}"
source_root="$workdir/GazeAnchor"
source_tarball="$topdir/SOURCES/gaze-anchor-$version.tar.gz"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'missing required command: %s\n' "$1" >&2
    exit 1
  fi
}

info() {
  printf '[info] %s\n' "$1"
}

require_cmd rpmbuild
require_cmd tar

mkdir -p "$topdir/SOURCES" "$topdir/SPECS" "$topdir/BUILD" "$topdir/BUILDROOT" "$topdir/RPMS" "$topdir/SRPMS"
rm -rf "$workdir"
mkdir -p "$source_root"

info "Staging working tree into $source_root"
cp -a "$repo_root/." "$source_root/"
rm -rf \
  "$source_root/.git" \
  "$source_root/.github" \
  "$source_root/.qt" \
  "$source_root/build" \
  "$source_root/CMakeCache.txt" \
  "$source_root/CMakeFiles" \
  "$source_root/cmake_install.cmake" \
  "$source_root/Makefile" \
  "$source_root/motion-stabilizer-linux" \
  "$source_root/motion-stabilizer-linux.desktop" \
  "$source_root/motion-stabilizer-linux_autogen" \
  "$source_root/gaze-anchor" \
  "$source_root/opcode-summary"

for path in "$source_root"/cmake-build-*; do
  if [ -e "$path" ]; then
    rm -rf "$path"
  fi
done

info "Creating source tarball $source_tarball"
tar -czf "$source_tarball" -C "$workdir" GazeAnchor
cp "$repo_root/packaging/gaze-anchor.spec" "$topdir/SPECS/"

info "Building RPM package"
rpmbuild -ba "$topdir/SPECS/gaze-anchor.spec" --define "_topdir $topdir"

if ! compgen -G "$topdir/RPMS/*/*.rpm" >/dev/null; then
  printf 'no binary RPM artifacts were produced in %s\n' "$topdir/RPMS" >&2
  exit 1
fi

info "RPM package artifacts"
for artifact in "$topdir"/RPMS/*/*.rpm "$topdir"/SRPMS/*.src.rpm; do
  if [ -e "$artifact" ]; then
    printf '  %s\n' "$artifact"
  fi
done
