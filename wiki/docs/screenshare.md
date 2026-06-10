# Screen Sharing

Screensharing in Wayland is done with the help of
[Portals](https://wiki.archlinux.org/title/XDG_Desktop_Portal). Portals were
initially designed to allow applications sandboxes via Flatpak to access
resources of the desktop, but have since gained popularity outside of the realm
of Flatpaks. In our case, we can use the screen sharing portal to allow
applications to access the contents of the Wayland compositor.


!!! note
    Please be aware that you will *only* be able to share a single screen with
    this method, not individual windows. Sharing individual windows requires
    the `ext_foreign_toplevel_image_capture_source_manager_v1` protocol, which
    Mir does not implement yet.


## Requirements

- miracle-wm built against MirAL >= 5.6 (Mir >= 2.26), which provides the
  `ext_image_copy_capture_v1` and `ext_image_capture_source_v1` protocols
- [xdg-desktop-portal-wlr](https://github.com/emersion/xdg-desktop-portal-wlr)
  \>= 0.8.0, which prefers those protocols when the compositor provides them
- `slurp`, used as the output chooser

!!! note
    Older versions of these instructions required building
    xdg-desktop-portal-wlr v0.5.0 from source, because later versions depended
    on dma-buf support in the compositor's `zwlr_screencopy_manager_v1`
    implementation that Mir lacks. This is no longer necessary: with the
    versions above, the portal uses `ext_image_copy_capture_v1` instead, so
    the package from your distribution works.

## Setup

### Installation
Install `xdg-desktop-portal-wlr` (>= 0.8.0) and `slurp` from your
distribution, for example:

```sh
sudo apt install xdg-desktop-portal-wlr slurp
```

If your distribution only packages an older version, build the latest release
from source:

```sh
git clone https://github.com/emersion/xdg-desktop-portal-wlr
cd xdg-desktop-portal-wlr
git checkout v0.8.2
meson setup builddir --prefix=/usr
cd builddir
ninja -j4
sudo ninja install
```

### Configuration
First, tell `xdg-desktop-portal` to use the `wlr` backend for screenshots and
screen casting by creating `~/.config/xdg-desktop-portal/portals.conf`:

```ini
[preferred]
org.freedesktop.impl.portal.Screenshot=wlr
org.freedesktop.impl.portal.ScreenCast=wlr
```

Next, create a file at `/etc/xdg/xdg-desktop-portal-wlr/config` with:
```ini
[screencast]
max_fps=30
chooser_type=simple
chooser_cmd=slurp -f %o -or
```

### Running
Build miracle with the systemd cmake flag: `-DSYSTEMD_INTEGRATION=1`. Among
other things, this ensures that `WAYLAND_DISPLAY` and `XDG_CURRENT_DESKTOP`
are imported into the D-Bus activation environment at startup, which the
portal needs in order to find the compositor.

Next, add the following startup application to miracle:

```yaml
# ~/.config/miracle-wm/config.yaml

startup_apps:
    - command: systemd-run --user /usr/libexec/xdg-desktop-portal --replace
      in_systemd_scope: true
```

This will make it so that your regular screensharing mechanism is disabled at
runtime in favor of our new one.

Finally, restart your compositor and then open up `obs-studio` or `Google Meet`
and see that you are able to share your screen.

### Troubleshooting
To verify that the portal is using the new capture path, kill any running
instance of `xdg-desktop-portal-wlr` and start it manually with tracing:

```sh
xdg-desktop-portal-wlr -l TRACE -r
```

It should log `wayland: using ext_image_copy_capture` on startup.
