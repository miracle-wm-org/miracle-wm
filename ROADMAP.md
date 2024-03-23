# Roadmap
The goal is to have an environment that I am comfortable daily-driving by **July 2024**.
This goal will be marked by the `1.0.0` release. Until then, all releases will be
`0.x.0`.

The dates attached to these releases aren't set in stone. Any release may come
earlier if the work is done. Also, new features may be added to the release while it's
in progress if priority changes, which may push back the release date.

# ✅ 0.1.0
**Proof of Concept (Due: February 17th)**

Features:
- [x] Window management
  - [x] Layout of new windows
  - [x] Delete window
  - [x] Resizing windows
  - [x] Gaps in windows
  - [x] Account for minimum sizes on windows (e.g. gedit)
  - [x] Handle exclusion zones
  - [x] Handle fullscreen windows
- [x] Navigation
  - [x] Changing selected window with keyboard
  - [x] Moving window with keyboard shortcuts
- [x] Outputs
  - [x] Output creation
  - [x] Output updating
  - [x] output deletion
- [x] Application quit command
- [x] Workspaces
  - [x] Workspace switching
  - [x] Moving windows between workspaces
- [x] Configuration
  - [x] Gap size
  - [x] Action key
  - [x] Startup apps
  - [x] Startup apps
  - [x] Override default keybindings
  - [x] Custom keybindings

# 🚧 0.2.0
**Stabilization + Usability (Due: April 15th)**

Features:
- [ ] Major bugs + performance
  - [ ] No crashes after a week of daily-driving
- [ ] Configuration
    - [x] Hot reloading
    - [ ] Display configuration
    - [x] Resize jump
    - [x] Outer gaps
- [x] Floating windows
  - [x] Floating window support (pop out of/into tiles, resizing, moving)
  - [x] "Pin to workspace" mode
- [x] I3 IPC support for workspaces
- [ ] Multi monitor support
  - [ ] Zero monitors
- [ ] Packaging
  - [x] Deb (jammy, mantic, noble, multi-architecture)
  - [ ] Fedora
  - [ ] Arch


# 0.3.0
**Pre 1.0.0 Release (Due: June 1st)**

This release relies on having the *Mir*'s renderer capabilities opened up to us.
If we can override the renderer, we can put *whatever we like* on the screen.

Features:
- [ ] Full I3 IPC integration
- [ ] Highlight border around selected window (carry over from **0.2.0**)
- [ ] Animation (requires access to Mir renderer)
  - [ ] Window movement interpolation
  - [ ] Window size interpolation
  - [ ] Workspace change
  - [ ] Move window to workspace
  - [ ] Opening
  - [ ] Closing
  - [ ] Configuration support
- [ ] Stacking windows

# 1.0.0
**Official Release (Due July 15th)**

Features:
- [ ] Bug free
- [ ] **IDEA**: Workspace/window overview view (similar to GNOME)
- [ ] **IDEA**: Settings app
- [ ] **IDEA**: Context menu on window with support for actions
- [ ] **IDEA**: Focus mode, where the focused application appears up front and center
- [ ] **IDEA**: "Picture in picture" mode (carry over from **0.2.0**)
- [ ] **IDEA**: A minimal default shell, including a panel, launcher, background, etc.