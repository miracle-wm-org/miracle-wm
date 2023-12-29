**WARNING**: This project is very much a work in progress. I have provided a rough roadmap in the timeline section.
Use at your own risk.

# miracle
**miracle** is a wayland tiling window manager based on [Mir](https://github.com/MirServer/mir). The tiling features
will be very sway/i3-like for the first iteration, but will diverge in some important ways later on. See the [timeline](#timeline)
section below for the current status and direction.

The ultimate goal of this work is to build an entire desktop envrionment on top of this window manager, but that will remain a
concern for a different repository with a different timeline.

# Building locally
```sh
git clone https://github.com/mattkae/miracle.git
cd miracle

mkdir build
cd build
cmake ..
./bin/miracle-wm
```

# Building the snap
```sh
cd miracle-wm
snapcraft
sudo snap install --dangerous --classic miracle-wm_*.snap
```

# Running

## On Login
Once installed, you may select the "Miracle WM" option from your display manager before you login (e.g. GDM or LightDM).
In most environments, this presents itself as a little "settings" button after you select your name.

## Hosted on your desktop
To run the window manager as a window in your current desktop session, simply run:
```sh
WAYLAND_DISPLAY=wayland-98 miracle-wm
```

Note that this is only useful if you want to test-drive the window manager or do some development on it for yourself.

# Timeline
## Tiling Window Manager Basics (Due: January 1st)
Version: 0.1

Features:
- [x] Layout new window
- [x] Delete window
- [x] Resizing windows
- [x] Changing selected window with keyboard
- [x] Moving window with keyboard
- [ ] Gaps in windows
- [ ] Account for minimum sizes on windows (e.g. gedit)
- [x] Disabling many of the Floating Window Manager features
- [ ] Handle exclusion zones
- [ ] Handle output creation, updating, and deletion

## Tiling Advanced(Due: February 1st)
Version: 0.2

Features:
- [ ] Workspaces
  - [ ] Moving windows between workspaces
