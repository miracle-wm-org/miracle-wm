# miracle-wm

**miracle-wm** is a hackable, stylish, and modern Wayland compositor built on [Mir](https://github.com/MirServer/mir). It brings the keyboard-driven efficiency of i3/Sway to a richer, more extensible experience with smooth animations, a powerful WebAssembly plugin system, and a clean YAML configuration.

See the [installation guide](getting_started/installation.md) for installation instructions on your platform.

## Features

- **Manual tiling** — organize windows into horizontal, vertical, tabbed, or stacked layouts with full keyboard control
- **Floating windows** — seamlessly mix floating and tiled windows in the same workspace
- **Smooth animations** — window open/close, moves, resizes, and workspace transitions all animate out of the box
- **WebAssembly plugin system** — extend and override core behavior without touching a line of C++
- **i3/Sway-compatible IPC** — works with `swaymsg`, Waybar, and the rest of the ecosystem you already know
- **Hot-reload config** — apply changes instantly with `Meta+Shift+R`, no restart required
- **Multi-monitor support** — independent workspaces across outputs, fully configurable
- **Accessibility built in** — magnifier, slow keys, sticky keys, and cursor configuration included

## Plugins

Plugins run as sandboxed [WebAssembly](https://webassembly.org/) modules, so you get first-class extensibility with zero risk to compositor stability.

Write plugins in **Rust** (via the [`miracle-plugin` crate](https://docs.miracle-wm.org/)) and hook into:

- Window placement logic — put windows exactly where you want them
- Input events — intercept keystrokes and pointer events before the compositor handles them
- Animations — trigger custom animations with arbitrary timing at any lifecycle event
- Workspace and output events — react to workspace creation, deletion, and switching
- Configuration — read and extend the compositor's config at runtime

Plugins hot-reload when the configuration reloads, so your iteration loop is fast.

## Links
- [Github Repository](https://github.com/miracle-wm-org/miracle-wm)
- [Trello Board](https://trello.com/b/ldzWTZDK/miracle)
- [Snapcraft.io listing](https://snapcraft.io/miracle-wm)
- [Fedora Miracle Spin](https://fedoraproject.org/spins/miraclewm/)
- [Mir](https://github.com/canonical/mir), the library that `miracle-wm` uses for all of the heavy-lifting
- [Miriway](https://github.com/Miriway/Miriway), another compositor built on top of *Mir*

!!! note
    If none of this makes any sense to you or you're new to Wayland or Linux, check out the [What is a Wayland compositor?](getting_started/what_is_a_wayland_compositor.md) document for a structured walk through the entire ecosystem.
