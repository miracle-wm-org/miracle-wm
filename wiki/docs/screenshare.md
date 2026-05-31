# Screen Sharing

Screensharing in Wayland is done with the help of
[Portals](https://wiki.archlinux.org/title/XDG_Desktop_Portal). Portals were
initially designed to allow applications sandboxes via Flatpak to access
resources of the desktop, but have since gained popularity outside of the realm
of Flatpaks. In our case, we can use the screen sharing portal to allow
applications to access the contents of the Wayland compositor.


!!! note
    Please be aware that you will *only* be able to share a single screen with
    this method, not individual windows.
    

## Setup

### Installation
First, we need to install
[xdg-desktop-portal-wlr](https://github.com/emersion/xdg-desktop-portal-wlr).
Unfortunately, Mir only has support for v0.5.0 of this software, so we will need
to checkout the source  at a particualr commit:

```sh
git clone https://github.com/emersion/xdg-desktop-portal-wlr
cd xdg-desktop-portal-wlr
git checkout 7c0f352  # This is release 0.5.0
```

Next, we can build the project:
```sh
meson setup builddir --prefix=/usr
cd builddir
ninja -j4
sudo ninja install
```

### Configuration
First, copy `/usr/share/xdg-desktop-portal/portals/wlr.portal` to
`/usr/share/xdg-desktop-portal/portals/mir.portal`

Next, modify `/usr/share/xdg-desktop-portal/portals/mir.portal`:

```diff
[portal]
DBusName=org.freedesktop.impl.portal.desktop.wlr
Interfaces=org.freedesktop.impl.portal.Screenshot;org.freedesktop.impl.portal.ScreenCast;
-UseIn=wlroots;sway;Wayfire;river
+UseIn=mir
```


Finally, create a file at `/etc/xdg/xdg-desktop-portal-wlr/config` with:
```ini
[screencast]
max_fps=30
chooser_type=simple
chooser_cmd=slurp -f %o -or
```

Note that you must have `slurp` installed on your machine.

### Running
Build miracle with the systemd cmake flag: `-DSYSTEMD_INTEGRATION=1`. Next, add
the following startup application to miracle:

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
