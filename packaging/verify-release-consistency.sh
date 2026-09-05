#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_root="${DIST_ROOT:-$repo_root/dist/release}"
expected_version="${1:-$(bash "$repo_root/packaging/version.sh" release-version)}"
verification_scope="${2:-packages}"

if [ "$verification_scope" != "packages" ] && [ "$verification_scope" != "source-only" ]; then
  printf 'usage: %s [version] [packages|source-only]\n' "$0" >&2
  exit 1
fi

fail() {
  printf '%s\n' "$1" >&2
  exit 1
}

info() {
  printf '[info] %s\n' "$1"
}

require_file() {
  if [ ! -f "$1" ]; then
    fail "missing required file: $1"
  fi
}

require_glob() {
  local matched=0
  for path in $1; do
    if [ -e "$path" ]; then
      matched=1
      break
    fi
  done
  if [ "$matched" -eq 0 ]; then
    fail "missing required release artifacts matching: $1"
  fi
}

require_optional_versioned_glob() {
  local pattern="$1"
  local description="$2"
  local matched=0
  local versioned=0
  for path in $pattern; do
    if [ -e "$path" ]; then
      matched=1
      case "$(basename "$path")" in
        *"$expected_version"*)
          versioned=1
          ;;
      esac
    fi
  done
  if [ "$matched" -eq 1 ] && [ "$versioned" -eq 0 ]; then
    fail "$description artifacts exist but filenames do not include version $expected_version"
  fi
}

project_version="$(bash "$repo_root/packaging/version.sh" project-version)"
if [ "$project_version" != "$expected_version" ]; then
  fail "project version mismatch: expected $expected_version, got $project_version"
fi

if [ -n "${GITHUB_REF_NAME:-}" ]; then
  tag_version="${GITHUB_REF_NAME#v}"
  if [ "$tag_version" != "$expected_version" ]; then
    fail "tag version mismatch: expected $expected_version, got $tag_version"
  fi
fi

require_file "$dist_root/RELEASE-NOTES.md"
require_file "$dist_root/SHA256SUMS"
require_file "$dist_root/source/gaze-anchor-$expected_version.tar.gz"
require_glob "$dist_root/source/*.tar.gz"

release_prefix="gaze-anchor-$expected_version"

deb_present=0
rpm_present=0
app_present=0

for path in "$dist_root"/deb/*; do
  if [ -e "$path" ]; then
    deb_present=1
    break
  fi
done

for path in "$dist_root"/rpm/*; do
  if [ -e "$path" ]; then
    rpm_present=1
    break
  fi
done

for path in "$dist_root"/appimage/*; do
  if [ -e "$path" ]; then
    app_present=1
    break
  fi
done

if [ "$verification_scope" = "packages" ] && [ "$deb_present" -eq 0 ] && [ "$rpm_present" -eq 0 ] && [ "$app_present" -eq 0 ]; then
  fail "release directory has no package artifacts in deb/, rpm/, or appimage/"
fi

require_optional_versioned_glob "$dist_root/deb/*.deb" "Debian package"
require_optional_versioned_glob "$dist_root/deb/*.changes" "Debian changes"
require_optional_versioned_glob "$dist_root/deb/*.buildinfo" "Debian buildinfo"
require_optional_versioned_glob "$dist_root/rpm/*.rpm" "RPM"
require_optional_versioned_glob "$dist_root/appimage/*.AppImage" "AppImage"
require_optional_versioned_glob "$dist_root/appimage/${release_prefix}.AppDir.tar.gz" "AppDir archive"

artifact_count="$(find "$dist_root" -maxdepth 2 -type f ! -name 'SHA256SUMS' | wc -l)"
checksum_count="$(wc -l < "$dist_root/SHA256SUMS")"

if [ "$artifact_count" -ne "$checksum_count" ]; then
  fail "checksum entry count mismatch: artifacts=$artifact_count checksums=$checksum_count"
fi

(
  cd "$dist_root"
  sha256sum -c SHA256SUMS
)

info "release consistency verified for version $expected_version"
