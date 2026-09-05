#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
  printf 'usage: %s <version> <date>\n' "$0" >&2
  printf 'example: %s 0.2.0 2026-09-05\n' "$0" >&2
  exit 1
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'missing required command: %s\n' "$1" >&2
    exit 1
  fi
}

version="${1:-}"
release_date="${2:-}"

if [ -z "$version" ] || [ -z "$release_date" ]; then
  usage
fi

if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  printf 'invalid version: %s\n' "$version" >&2
  exit 1
fi

case "$release_date" in
  [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]) ;;
  *)
    printf 'invalid date: %s\n' "$release_date" >&2
    exit 1
    ;;
esac

require_cmd sed
require_cmd date

rfc2822_date="$(date -d "$release_date" '+%a, %d %b %Y 12:00:00 +0000')"
rpm_changelog_date="$(date -d "$release_date" '+%a %b %d %Y')"

sed -i -E "s/^project\(GazeAnchor VERSION [0-9]+\.[0-9]+\.[0-9]+ LANGUAGES CXX\)$/project(GazeAnchor VERSION $version LANGUAGES CXX)/" \
  "$repo_root/CMakeLists.txt"

sed -i -E "s/^Current version: \`[^\`]+\`$/Current version: \`$version\`/" \
  "$repo_root/README.md"

sed -i -E "s#<release version=\"[0-9]+\.[0-9]+\.[0-9]+\" date=\"[0-9-]+\" />#<release version=\"$version\" date=\"$release_date\" />#" \
  "$repo_root/packaging/io.github.gazeanchor.GazeAnchor.metainfo.xml"

sed -i -E "s/^Version:[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+$/Version:        $version/" \
  "$repo_root/packaging/gaze-anchor.spec"

sed -i -E "s/^\* .+ GazeAnchor Maintainers <opensource@example\.invalid> - [0-9]+\.[0-9]+\.[0-9]+-1$/\* $rpm_changelog_date GazeAnchor Maintainers <opensource@example.invalid> - $version-1/" \
  "$repo_root/packaging/gaze-anchor.spec"

cat > "$repo_root/debian/changelog" <<EOF
gaze-anchor ($version-1) unstable; urgency=medium

  * Prepare release $version.

 -- GazeAnchor Maintainers <opensource@example.invalid>  $rfc2822_date
EOF

printf '[info] updated release version to %s\n' "$version"
