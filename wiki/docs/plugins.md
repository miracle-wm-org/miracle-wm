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
- **IPC** — handle custom commands from, and publish events to, IPC clients (see [IPC integration](#ipc-integration))

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

## IPC integration

A plugin can expose itself over miracle-wm's [IPC socket](ipc/index.md) under a **namespace** it
owns. This gives IPC clients a bidirectional, plugin-defined JSON channel: clients send commands
to the plugin, and the plugin publishes events back to interested clients.

- A namespace is a plain string. Each namespace is owned by at most one plugin, and each plugin
  may own at most one namespace. Registering a namespace that is already taken, or registering a
  second namespace from the same plugin, fails.
- Only [PLUGIN_COMMAND](ipc/plugin_command.md) messages whose `plugin` field matches the
  registered namespace are routed to the plugin.
- Only clients subscribed to the [plugin](ipc/events/plugin.md) event **for that namespace**
  receive the events the plugin publishes.

### Registering a namespace

Claim a namespace once (typically from `init`):

```rust
use miracle_plugin::plugin::register_namespace;

register_namespace("my-plugin").expect("namespace already taken");
```

### Handling commands

Implement `handle_command` on your `Plugin`. It receives the raw JSON string sent by the client
under the command's `payload` field, and returns `Some(response_json)` to reply, or `None` to
decline (the client receives `success: false`):

```rust
fn handle_command(&mut self, payload: &str) -> Option<String> {
    // ...parse payload, do work...
    Some(r#"{"ok": true}"#.to_string())
}
```

### Publishing events

Push an arbitrary JSON payload to every client subscribed to your namespace:

```rust
use miracle_plugin::plugin::publish_event;

publish_event(r#"{"state": "connected"}"#).ok();
```

Clients receive it as a [plugin](ipc/events/plugin.md) event wrapped as
`{ "plugin": "my-plugin", "payload": { "state": "connected" } }`.

## Examples

- **[`miri-plugin` crate](https://github.com/miracle-wm-org/miri-plugin)** — the Miri plugin that turns any Miracle compositor into a scrolling window manager
- **[mattkae/dotfiles](https://github.com/mattkae/dotfiles/tree/master/config/miracle-wm/matts-config)** — a real-world plugin used in the author's personal setup

## Configuring Plugins
Plugins still need to be specified in your configuration before they are loaded into Miracle. See [configuration/plugins.md](configuration/plugins.md) for how to load plugins and pass user data to them from your `miracle-wm/config.yaml`.
