#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
parent_dir="$(dirname "$repo_root")"

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

require_cmd dpkg-deb

package_path="${1:-}"
if [ -z "$package_path" ]; then
  matches=("$parent_dir"/gaze-anchor_*.deb)
  if [ ! -e "${matches[0]}" ]; then
    printf 'no .deb artifacts found in %s\n' "$parent_dir" >&2
    exit 1
  fi
  package_path="${matches[0]}"
fi

tmp_list="$(mktemp)"
trap 'rm -f "$tmp_list"' EXIT

dpkg-deb -c "$package_path" | awk '{print $6}' > "$tmp_list"

require_pattern "./usr/bin/gaze-anchor" "$tmp_list"
require_pattern "./usr/share/applications/io.github.gazeanchor.GazeAnchor.desktop" "$tmp_list"
require_pattern "./usr/share/icons/hicolor/scalable/apps/io.github.gazeanchor.GazeAnchor.svg" "$tmp_list"
require_pattern "./usr/share/metainfo/io.github.gazeanchor.GazeAnchor.metainfo.xml" "$tmp_list"
require_pattern "./usr/share/licenses/gaze-anchor/LICENSE" "$tmp_list"

printf '[info] verified Debian package contents: %s\n' "$package_path"
