<p align="center">
  <a href="https://miracle-wm.org/">
    <img alt="Website" src="https://img.shields.io/badge/Visit-Website-blue?style=for-the-badge&logo=google-chrome&logoColor=white"/>
  </a>
  &nbsp;
  <a href="https://matrix.to/#/#miracle-wm:matrix.org">
    <img alt="Join Matrix" src="https://img.shields.io/badge/Chat%20on-Matrix-2D7D46?logo=matrix&logoColor=white&style=for-the-badge"/>
  </a>
</p>

# miracle-wm

**miracle-wm** is a hackable, stylish, and modern Wayland compositor built on [Mir](https://github.com/MirServer/mir). It brings the keyboard-driven efficiency of i3/Sway to a richer, more extensible experience with smooth animations, a powerful WebAssembly plugin system, and a clean YAML configuration.

![miracle in action](./resources/screenshot1.png "miracle in action")

## Features

Most tiling compositors make you choose between productivity and polish. miracle-wm doesn't.

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

## Getting Started

Installation instructions, build instructions, and a full roadmap live at:

**[wiki.miracle-wm.org](https://wiki.miracle-wm.org/latest/)**

Plugin API documentation is at:

**[docs.miracle-wm.org](https://docs.miracle-wm.org/)**

## Community

Questions, ideas, and contributions are welcome. Find us on Matrix:

**[#miracle-wm:matrix.org](https://matrix.to/#/#miracle-wm:matrix.org)**
