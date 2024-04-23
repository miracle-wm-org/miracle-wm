Name:           miracle-wm
Version:        0.2.1
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
%{_vpath_builddir}/bin/miracle-wm-tests
desktop-file-validate %{buildroot}%{_datadir}/wayland-sessions/miracle-wm.desktop


%files
%{_bindir}/miracle-wm
%{_bindir}/miracle-wm-sensible-terminal
%{_datarootdir}/wayland-sessions/miracle-wm.desktop
%license LICENSE


%changelog
* Tue Apr 23 2024 Matthew Kosarek <matt.kosarek@canonical.com> - 0.2.1-1
- Release for the deb and Fedora packages

* Mon Apr 22 2024 Matthew Kosarek <matt.kosarek@canonical.com> - 0.2.0-1
- (#35) sway/i3 IPC support has been implemented to minimally support waybar
- (#45) Added "floating window manager" support whereby individual windows can be made to float above the tiling grid and behave just as they would in a "traditional" floating window manager
- (#38) The user configuration now automatically reloads when a change is made to it
- (#37) A terminal option can now be specified in the configuration to decide which terminal is opened up by the keybind. We also do a much better job of deciding on a sane default terminal
- Environment variables can now be specified in the configuration (e.g. I needed to set mesa_glthread=false to prevent a bunch of screen tearing on my new AMD card)
- Upgrade to Mir v2.16.4 which brought in a few important bugfixes for miracle-wm
- (#48) Fullscreened windows are now guaranteed to be on top
- (#34) Fixed a bug where panels could not be interacted with
- (#50) Keyboard events are now properly consumed when a workspace switch happens
- (#61) Outer gaps no longer include inner gaps
- (#66) Disabled moving fullscreen windows between workspaces
- (#67) Fixed a bug where resizing a window over and over again would make it progressively tinier due to rounding errors
- Refactored the tiling window system in a big way for readability. This solved a number of tricky bugs in the process so I am very happy about it
- (#81) Gaps algorithm no longer leaves some nodes smaller than others
- The project finally has meaningful tests with many more to come 🧪

* Mon Apr 01 2024 Matthew Kosarek <matt.kosarek@canonical.com> - 0.1.0-1
- Initial version