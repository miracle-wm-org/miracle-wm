# Roadmap
The goal is to have an environment that I am comfortable daily-driving by **July 2024**.
This goal will be marked by the `1.0.0` release. Until then, all releases will be
`0.x.0`.

The dates attached to these releases aren't set in stone. Any release may come
earlier if the work is done. Also, new features may be added to the release while it's
in progress if priority changes, which may push back the release date.

# 0.1.0
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

# 0.2.0
**Stabilization + Usability (Due: April 1st)**

Features:
- [ ] Major bugs + performance
- [ ] Configuration File
    - [x] Hot reloading
    - [ ] Display configuration
    - [ ] Resize jump
    - [x] Outer gaps
- [ ] Floating windows overlayed
  - [ ] "Pin to workspace" mode
  - [ ] Support for "picture-in-picture" mode
- [x] I3 IPC support for workspaces
- [ ] Highlight border around selected window (Difficult!)
- [ ] Improve multi monitor support
  - [ ] Zero monitors
  - [ ] Fix bugs for monitors going online and offline


# 0.3.0
**Pre 1.0.0 Release (Due: June 1st)**

Features:
- [ ] Full I3 IPC integration
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
- [ ] Major bugs and major usability
- [ ] **IDEA**:Workspace/window overview view (similar to GNOME)
- [ ] **IDEA**: Settings app
