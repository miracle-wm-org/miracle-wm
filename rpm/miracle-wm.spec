Name:           miracle-wm
Version:        0.0.1
Release:        1%{?dist}
Summary:        miracle-wm is a tiling Wayland compositor based on Mir 

License:        GPL0-3.0
URL:            https://github.com/mattkae/miracle-wm
Source0:        %{name}-%{version}.tar.gz
BuildArch:      noarch

BuildRequires:  mir-devel, cmake, g++, glib2, glib2-devel, yaml-cpp-devel, libevdev-devel, json-devel, libnotify-devel, gtest, gtest-devel, libxkbcommon-devel

%description
miracle-wm is a Wayland compositor based on Mir. It features a tiling window manager at its core, very much in the style of i3 and sway. The intention is to build a compositor that is flashier and more feature-rich than either of those compositors, like swayfx.

%prep
%autosetup


%build
%cmake
%cmake_build


%install
%global __brp_check_rpaths %{nil}
%cmake_install


%changelog
* Thu Mar 28 2024 mk
- First Test
