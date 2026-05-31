# Building

## Dependencies

- [cmake](https://cmake.org/) >= 3.7
- [gcc](https://gcc.gnu.org/) or [clang](https://clang.llvm.org/) with C++23 support
- [miral](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/) >= 7
- [mirplatform](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/)
- [mirwayland](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/)
- [mirserver](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/)
- [mirserver-internal](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/)
- [mircommon](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/)
- [mir-graphics-drivers-desktop](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/) >= 2.14
- [mir-graphics-drivers-nvidia](https://canonical-mir.readthedocs-hosted.com/stable/tutorial/) >= 2.14 (NVIDIA Only)
- [glib-2.0](https://docs.gtk.org/glib/)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
- [libevdev](https://www.freedesktop.org/wiki/Software/libevdev/)
- [nlohmann json](https://github.com/nlohmann/json) >= 3.2.0
- [libnotify](https://gitlab.gnome.org/GNOME/libnotify)
- [libxkbcommon](https://github.com/xkbcommon/libxkbcommon)
- [libgles2-mesa-dev](https://docs.mesa3d.org/opengles.html)
- [json-c](https://github.com/json-c/json-c) >= 0.17
- [dbus-next](https://pypi.org/project/dbus-next/)
- [tenacity](https://pypi.org/project/tenacity/)
- [wasmedge](https://wasmedge.org/)

## From source

```sh
# If on Debian or a derivative, first install Mir from the mir-team/release ppa:
sudo add-apt-repository ppa:mir-team/release
sudo apt update
sudo apt install libmiral-dev libmircommon-dev libmirserver-internal-dev \
  libgtest-dev libyaml-cpp-dev libglib2.0-dev libevdev-dev nlohmann-json3-dev libnotify-dev pcre2-utils \
  libmiroil-dev libmirplatform-dev libgles2-mesa-dev libmirwayland-dev libjson-c-dev libgtest-dev libgmock-dev \
  mirtest-dev mirtest-internal-dev mir-wlcs-integration libxkbcommon-dev libboost-filesystem-dev libboost-system-dev \
  xwayland mir-platform-graphics-gbm-kms mir-platform-rendering-egl-generic

# If running on a desktop, you will also want to add the desktop graphics drivers.
sudo apt install mir-graphics-drivers-desktop

# Then, clone the repo:
git clone https://github.com/miracle-window-manager/miracle-wm.git
cd miracle-wm

# Next, build a debug build...
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# ... or a max-performance release build:
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=ON
cmake --build build

# Finally run with:
WAYLAND_DISPLAY=wayland-98 ./build/miracle-wm
```

### CMake Options

The following options are available at build time:

- `-DSNAP_BUILD`: Informs cmake that this is being built for a snap
  since some parameters need to be tweaked for that use case.
- `-DSYSTEMD_INTEGRATION`: If enabled, miracle will build with full
  systemd integration, including establishing a user session and
  importing the proper environment variables in to systemd.
- `DENABLE_TESTS`: If enabled, tests are built (default=true)
- `-DEND_TO_END_TESTS`: If enabled, miracle's end-to-end tests will
  be compiled as part of the test suite.

### Feature Flags

- `-DFEATURE_PLUGIN_SYSTEM`: if 1, the experimental WebAssembly
  plugin system will also be enabled.
- `-DFEATURE_MULTI_SELECT`: if 1, the experimental multi select
  functionality will be enabled.
- `-DFEATURE_PARENT_CONTAINER_WALLPAPERS`: if 1, the experimental
  background on floating parent containers will be enabled.

## Snap

```sh
cd miracle-wm
snapcraft
sudo snap install --dangerous --classic miracle-wm_*.snap
```
