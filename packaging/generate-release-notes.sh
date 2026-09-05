#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_root="${DIST_ROOT:-$repo_root/dist/release}"
version="$(bash "$repo_root/packaging/version.sh" release-version)"
notes_file="$dist_root/RELEASE-NOTES.md"

mkdir -p "$dist_root"

cat > "$notes_file" <<EOF
# GazeAnchor $version

## Artifacts

- Source tarball: \`source/gaze-anchor-$version.tar.gz\`
- Debian packages: see files under \`deb/\`
- RPM packages: see files under \`rpm/\`
- AppDir staging bundle: optional, generated when AppImage staging runs

## Notes

- KDE Plasma Wayland and X11 remain the primary tested targets.
- Debian and RPM native packages currently build the X11 profile for broader distro compatibility.
- AppImage release currently ships as an AppDir archive until linuxdeployqt/appimagetool are integrated into release automation.

## Verification

Checksums are listed in \`SHA256SUMS\`.

Use:

\`\`\`bash
sha256sum -c SHA256SUMS
\`\`\`

to verify downloaded artifacts.
EOF

printf '%s\n' "$notes_file"
