#!/usr/bin/env bash
set -e

SNAP=/workspace/miracle-snap
ARCH_TRIPLET=$(dpkg-architecture -qDEB_HOST_MULTIARCH)

export SNAP
export PATH="$SNAP/bin:$SNAP/usr/bin:$SNAP/usr/local/bin:$PATH"
export LD_LIBRARY_PATH="$SNAP/usr/lib/$ARCH_TRIPLET:$SNAP/lib/$ARCH_TRIPLET:$SNAP/usr/lib:$SNAP/lib:${LD_LIBRARY_PATH:-}"
export MIR_SERVER_PLATFORM_PATH="$SNAP/usr/lib/$ARCH_TRIPLET/mir/server-platform"
export __EGL_VENDOR_LIBRARY_DIRS="$SNAP/etc/glvnd/egl_vendor.d:$SNAP/usr/share/glvnd/egl_vendor.d"
export GBM_BACKENDS_PATH="$SNAP/usr/lib/$ARCH_TRIPLET/gbm"
export LIBGL_DRIVERS_PATH="$SNAP/usr/lib/$ARCH_TRIPLET/dri"
export LIBINPUT_QUIRKS_DIR="$SNAP/usr/share/libinput"
export DRIRC_CONFIGDIR="$SNAP/usr/share/drirc.d"

exec "$SNAP/usr/bin/miracle-wm" "$@"
