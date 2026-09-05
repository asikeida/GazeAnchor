#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
parent_dir="$(dirname "$repo_root")"
package_name="gaze-anchor"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'missing required command: %s\n' "$1" >&2
    exit 1
  fi
}

info() {
  printf '[info] %s\n' "$1"
}

require_cmd dpkg-buildpackage
require_cmd dpkg-parsechangelog

info "Building Debian package skeleton from $repo_root"
dpkg-parsechangelog -l"$repo_root/debian/changelog" >/dev/null

cd "$repo_root"
dpkg-buildpackage -us -uc -b

artifacts=("$parent_dir"/${package_name}_*.deb)
if [ ! -e "${artifacts[0]}" ]; then
  printf 'no .deb artifacts were produced in %s\n' "$parent_dir" >&2
  exit 1
fi

info "Debian package artifacts"
for artifact in "$parent_dir"/${package_name}_*.deb "$parent_dir"/${package_name}_*.changes "$parent_dir"/${package_name}_*.buildinfo; do
  if [ -e "$artifact" ]; then
    printf '  %s\n' "$artifact"
  fi
done
