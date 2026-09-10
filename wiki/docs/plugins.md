# Plugins

miracle-wm's plugin system lets you extend and override core compositor behavior through sandboxed [WebAssembly](https://webassembly.org/) modules. Because plugins run in a WebAssembly sandbox, a misbehaving plugin cannot crash or corrupt the compositor.

## Writing a Plugin

Plugins are written in **Rust** using the [`miracle-plugin`](https://crates.io/crates/miracle-plugin) crate. The crate provides macros and types for hooking into compositor lifecycle events. Other languages may be supported in the future in addition to Rust.

Full API documentation is available at [docs.miracle-wm.org/miracle_plugin/](https://docs.miracle-wm.org/miracle_plugin/).

## Examples

- **[miri](https://github.com/miracle-wm-org/miri-plugin)** — the Miri plugin that turns  Miracle into a scrolling window manager
- **[focus-blur-plugin](https://github.com/miracle-wm-org/focus-blur-plugin)** — a plugin that blurs unfocused windows
- **[night-light-plugin](https://github.com/miracle-wm-org/focus-blur-plugin)** — a plugin that applies an orange tint to your screen as the day progresses
- **[mattkae/dotfiles](https://github.com/mattkae/dotfiles/tree/master/config/miracle-wm/matts-config)** — a real-world plugin used in the author's personal setup

## Loading Plugins
To load your plugin, you can either specify them in your configuration
(see [plugins](configuration/plugins.md)).

Alternatively, you can place the `.wasm` file directly into 
`$XDG_CONFIG_HOME/miracle-wm/config/plugins` and they will be loaded directly.
