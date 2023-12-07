# Miracle - a Tiling WM based on Mir
**Miracle** is a pleasant tiling window manager based on *Mir*. The emphasis is on usability for normal people,
rather than maximum extensibility for hackers. My goal is for everyone to enjoy the beauty of tiling window managers
without needing to be a nerd.

## Building
```
git clone https://github.com/mattkae/miracle.git
cd miracle

mkdir build
cd build
cmake ..
./bin/compositor
```

## TODOs V1
**ETA**: January 1st
- [x] Layout new window
- [x] Delete window
- [ ] Resizing windows
- [ ] Changing selected window with keyboard
- [ ] Moving window with keyboard
- [ ] Gaps in windows
- [ ] Account for minimum sizes on windows (e.g. gedit)
- [ ] Disabling many of the Floating Window Manager features

## TODOs V2
**ETA**: February 1st
- [ ] Workspaces
  - [ ] Moving windows between workspaces
- [ ] Multi-output support
  - [ ] Connecting new monitors
  - [ ] Disconnecting monitors
  - [ ] Moving windows between monitors

## TODOs V3
**ETA**: March 1st
- [ ] Configuration file
- [ ] Settings application (built in flutter ideally)
- [ ] Visual Sugar
  - [ ] A workspace visualizer in the top panel (in-memory app)
  - [ ] A tile visualizer in the top panel (in-memory app)