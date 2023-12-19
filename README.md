# miracle
**miracle** is a _Mir_-based wayland desktop environment inspired by modern IDEs like CLion. The window manager intends
to provide a typical tiling window management experience (akin to i3 or sway). The shell itself focuses on extendability
for power users while at the same time having an accessible user interface. While window managers like i3 and sway
are simple and easy-to-use, I find that there's a niche missing for programmers who want an out-of-the-box solution
to an ergonomic but heavy-hitter desktop environment.

**WARNING**: This project is very much a work in progress. I have provided a rough roadmap in the timeline section.
Use at your own risk.

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
- [ ] Disabling many of the Floating Window Manager features
- [ ] Handle exclusion zones
- [ ] Handle output creation, updating, and deletion
- [ ] 

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

## Tiling More Advanced + 
Version: 0.3

Features:
- [ ] UI Cleanup
- [ ] Configuration file
- [ ] Settings application
- [ ] Visual Sugar
  - [ ] A workspace visualizer in the top panel (in-memory app)
  - [ ] A tile visualizer in the top panel (in-memory app)