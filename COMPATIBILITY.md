# Linux Compatibility

Motion Stabilizer Linux can be built on more Linux systems now, but feature availability depends on the display server and desktop environment.

## Support Matrix

| Environment | Build | Overlay | Click-through | Global Hotkeys | Notes |
|-------------|-------|---------|---------------|----------------|-------|
| KDE Plasma Wayland | Full | Best | Best effort via layer-shell/Qt | Yes, via KGlobalAccel | Primary target |
| KDE Plasma X11 | Full | Good | Yes, via XFixes | Yes, via KGlobalAccel | Good fallback |
| Generic X11 | Full or X11-only | Good | Yes, via XFixes | Only if KGlobalAccel is enabled/available | Recommended non-KDE path |
| GNOME Wayland | Minimal/full may build | Not guaranteed | Not guaranteed | No generic support | Wayland policy blocks many overlay behaviors |
| Sway/Hyprland/wlroots | Needs compositor-specific validation | Possible with layer-shell alternatives | Depends on compositor | Usually needs portal/compositor config | Not implemented as a dedicated backend |
| Gamescope/Steam Deck | Builds | Needs validation | Needs validation | Needs validation | Depends whether overlay runs inside or outside gamescope |

## Build Profiles

Full KDE-oriented build:

```bash
cmake -S linux -B /tmp/motion-stabilizer-linux-build \
  -DENABLE_LAYER_SHELL_QT=ON \
  -DENABLE_KGLOBALACCEL=ON \
  -DENABLE_X11_FALLBACK=ON
cmake --build /tmp/motion-stabilizer-linux-build -j
```

Minimal Qt-only build:

```bash
cmake -S linux -B /tmp/motion-stabilizer-linux-minimal-build \
  -DENABLE_LAYER_SHELL_QT=OFF \
  -DENABLE_KGLOBALACCEL=OFF \
  -DENABLE_X11_FALLBACK=OFF
cmake --build /tmp/motion-stabilizer-linux-minimal-build -j
```

The minimal build is useful for portability testing, but it does not provide reliable Wayland overlay stacking or global shortcuts.

## Diagnostics

Run:

```bash
motion-stabilizer-linux --diagnose
```

Check:

- `Qt platform`
- screen list and DPR
- runtime files
- compiled features:
  - `HAVE_LAYER_SHELL_QT`
  - `HAVE_KGLOBALACCEL`
  - `HAVE_X11_FALLBACK`

## Practical Distribution Guidance

- For Arch/KDE users, use the provided `PKGBUILD`.
- For KDE Flatpak, use the provided KDE runtime manifest.
- For generic Linux, AppImage needs linuxdeploy/linuxdeployqt work before it is a true portable binary.
- For GNOME Wayland and wlroots compositors, a separate backend or portal-based shortcut implementation is required for good support.
