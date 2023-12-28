**WARNING**: This project is very much a work in progress. I have provided a rough roadmap in the timeline section.
Use at your own risk.

# miracle
**miracle** is a _Mir_-based, Wayland desktop environment inspired by modern IDEs. The window manager intends
to provide a typical tiling window management experience (akin to i3 or sway). The shell focuses on usability for
programmers by being extendable and providing a JetBrains-inspried interface. While window managers like i3 and sway
are simple but barebones, miracle will provide a heavier out-of-the-box-solution that is geared towards making
the life of the average programmer happy and productive. If you like the lightweight nature of other IDEs, then this
one might not be for you. But if you want your desktop environment to provide a host of useful facilities at your
fingertips, then look no futher!

While **miracle** intends to be a full desktop environment, the tiling window management facilities will always
be provided as a standalone option so that you might use your own shell if you like.

# Building
```
git clone https://github.com/mattkae/miracle.git
cd miracle

mkdir build
cd build
cmake ..
./bin/compositor
```

# Timeline
## Tiling Window Manager (Due: January 1st)
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

## Tiling Advanced + Initial Shell (Due: February 1st)
Version: 0.2

Features:
- [ ] Workspaces
  - [ ] Moving windows between workspaces
- [ ] Launcher (Rust + GTK most likely)
  - [ ] Favorite applications
  - [ ] Opened applications
  - [ ] Favorite folders
  - [ ] Pinned scripts
- [ ] Top Panel
  - [ ] Clock
  - [ ] Battery indicator
  - [ ] Wifi Indicator
  - [ ] Lock Screen
  - [ ] Weather

## Tiling More Advanced + Futher UI
Version: 0.3

Features:
- [ ] UI Cleanup
- [ ] UI Plugin Interface
- [ ] Configuration file
- [ ] Settings application
- [ ] Visual Sugar
  - [ ] A workspace visualizer in the top panel (in-memory app)
  - [ ] A tile visualizer in the top panel (in-memory app)