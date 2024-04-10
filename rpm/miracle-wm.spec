Name:           miracle-wm
Version:        0.1.0
Release:        1%{?dist}
Summary:        A tiling Wayland compositor based on Mir 

License:        GPL-3.0-or-later
URL:            https://github.com/mattkae/miracle-wm
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(miral)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(yaml-cpp)
BuildRequires:  pkgconfig(libevdev)
BuildRequires:  cmake(nlohmann_json) >= 3.2.0
BuildRequires:  pkgconfig(libnotify)
BuildRequires:  cmake(gtest)
BuildRequires:  libxkbcommon-devel
BuildRequires:  desktop-file-utils

%description
miracle-wm is a Wayland compositor based on Mir. It features a tiling window 
manager at its core, very much in the style of i3 and sway. The intention is 
to build a compositor that is flashier and more feature-rich than either of 
those compositors, like swayfx.

%prep
%autosetup


%build
%cmake
%cmake_build


%install
%cmake_install


%check
%ctest
desktop-file-validate %{buildroot}%{_datadir}/wayland-sessions/miracle-wm.desktop


%files
%{_bindir}/miracle-wm
%{_bindir}/miracle-wm-sensible-terminal
%{_datarootdir}/wayland-sessions/miracle-wm.desktop
%license LICENSE


%changelog
* Mon Apr 01 2024 Matthew Kosarek <matt.kosarek@canonical.com> - 0.1.0-1
- Initial version
