Name:           miracle-wm
Version:        0.0.1
Release:        1%{?dist}
Summary:        A tiling Wayland compositor based on Mir 

License:        GPLv3+
URL:            https://github.com/mattkae/miracle-wm
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  g++
BuildRequires:  mir-devel
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  glib2-devel
BuildRequires:  yaml-cpp-devel
BuildRequires:  libevdev-devel
BuildRequires:  json-devel
BuildRequires:  libnotify-devel
BuildRequires:  cmake(gtest)
BuildRequires:  gtest-devel
BuildRequires:  libxkbcommon-devel

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

%files
%{_bindir}/miracle-wm
%{_bindir}/miracle-wm-sensible-terminal
%{_datarootdir}/wayland-sessions/miracle-wm.desktop


%changelog
* Thu Mar 28 2024 mk
- First Test
