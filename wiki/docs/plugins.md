# Plugins

miracle-wm's plugin system lets you extend and override core compositor behavior through sandboxed [WebAssembly](https://webassembly.org/) modules. Because plugins run in a WebAssembly sandbox, a misbehaving plugin cannot crash or corrupt the compositor.

## Writing a Plugin

Plugins are written in **Rust** using the [`miracle-plugin`](https://crates.io/crates/miracle-plugin) crate. The crate provides macros and types for hooking into compositor lifecycle events. Other languages may be supported in the future in addition to Rust.

Full API documentation is available at [docs.miracle-wm.org/miracle_plugin/](https://docs.miracle-wm.org/miracle_plugin/).

### What you can hook into

- **Window placement** — control where new windows are placed
- **Input events** — intercept keystrokes and pointer events before the compositor handles them
- **Animations** — trigger custom animations at any lifecycle event
- **Workspace and output events** — react to workspace creation, deletion, and switching
- **Configuration** — programmatically specify the configuration as a plugin data at runtime

### Getting started

Add the crate to your `Cargo.toml`:

```toml
[dependencies]
miracle-plugin = "*"

[lib]
crate-type = ["cdylib"]
```

Then compile to WebAssembly:

```sh
rustup target add wasm32-wasip1  # This should be done once
cargo build --target wasm32-wasip2 --release
```

The resulting `.wasm` file is what you load in your miracle-wm configuration.

## Examples

- **[`miri-plugin` crate](https://github.com/miracle-wm-org/miri-plugin)** — the Miri plugin that turns any Miracle compositor into a scrolling window manager
- **[mattkae/dotfiles](https://github.com/mattkae/dotfiles/tree/master/config/miracle-wm/matts-config)** — a real-world plugin used in the author's personal setup

## Configuring Plugins
Plugins still need to be specified in your configuration before they are loaded into Miracle. See [configuration/plugins.md](configuration/plugins.md) for how to load plugins and pass user data to them from your `miracle-wm/config.yaml`.
