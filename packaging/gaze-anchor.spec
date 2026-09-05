Name:           gaze-anchor
Version:        0.1.0
Release:        1%{?dist}
Summary:        Visual stabilization overlay for Linux desktop sessions

License:        MIT
URL:            https://github.com/zjut-gwb/MotionStabilizer-Linux
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconf-pkg-config
BuildRequires:  pkgconfig(Qt6Widgets)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(xext)

Requires:       qt6-qtbase

%description
GazeAnchor provides non-invasive visual anchors such as edge overlays,
a center crosshair, and a floating clock to reduce motion sickness in
3D games.

This initial RPM packaging skeleton builds the X11 profile for wider
Fedora and openSUSE compatibility. KDE Wayland-specific support based
on LayerShellQt and KGlobalAccel will require additional target-distro
dependency work before a full-featured RPM package is advertised.

%prep
%autosetup -n GazeAnchor

%build
%cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_PROFILE=X11 \
  -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_bindir}/gaze-anchor
%{_datadir}/applications/io.github.gazeanchor.GazeAnchor.desktop
%{_datadir}/icons/hicolor/scalable/apps/io.github.gazeanchor.GazeAnchor.svg
%{_datadir}/metainfo/io.github.gazeanchor.GazeAnchor.metainfo.xml
%{_datadir}/licenses/gaze-anchor/LICENSE

%changelog
* Fri Sep 04 2026 GazeAnchor Maintainers <opensource@example.invalid> - 0.1.0-1
- Initial RPM packaging skeleton
