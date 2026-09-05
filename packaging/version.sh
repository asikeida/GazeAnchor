#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

project_version() {
  grep -E '^project\(GazeAnchor VERSION [0-9]+\.[0-9]+\.[0-9]+' "$repo_root/CMakeLists.txt" \
    | sed -E 's/^project\(GazeAnchor VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/'
}

release_version() {
  if [ -n "${RELEASE_VERSION:-}" ]; then
    printf '%s\n' "$RELEASE_VERSION"
    return 0
  fi
  project_version
}

release_prefix() {
  printf 'gaze-anchor-%s\n' "$(release_version)"
}

case "${1:-}" in
  project-version)
    project_version
    ;;
  release-version)
    release_version
    ;;
  release-prefix)
    release_prefix
    ;;
  *)
    printf 'usage: %s {project-version|release-version|release-prefix}\n' "$0" >&2
    exit 1
    ;;
esac
