#!/usr/bin/env bash
set -euo pipefail

topdir="${RPM_TOPDIR:-/tmp/opencode/gaze-anchor-rpmbuild}"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'missing required command: %s\n' "$1" >&2
    exit 1
  fi
}

require_pattern() {
  local pattern="$1"
  local file="$2"
  if ! grep -Fqx "$pattern" "$file"; then
    printf 'missing expected package entry: %s\n' "$pattern" >&2
    exit 1
  fi
}

require_cmd rpm

package_path="${1:-}"
if [ -z "$package_path" ]; then
  matches=("$topdir"/RPMS/*/gaze-anchor-*.rpm)
  if [ ! -e "${matches[0]}" ]; then
    printf 'no .rpm artifacts found in %s\n' "$topdir/RPMS" >&2
    exit 1
  fi
  package_path="${matches[0]}"
fi

tmp_list="$(mktemp)"
trap 'rm -f "$tmp_list"' EXIT

rpm -qlp "$package_path" > "$tmp_list"

require_pattern "/usr/bin/gaze-anchor" "$tmp_list"
require_pattern "/usr/share/applications/io.github.gazeanchor.GazeAnchor.desktop" "$tmp_list"
require_pattern "/usr/share/icons/hicolor/scalable/apps/io.github.gazeanchor.GazeAnchor.svg" "$tmp_list"
require_pattern "/usr/share/metainfo/io.github.gazeanchor.GazeAnchor.metainfo.xml" "$tmp_list"
require_pattern "/usr/share/licenses/gaze-anchor/LICENSE" "$tmp_list"

printf '[info] verified RPM package contents: %s\n' "$package_path"
