# miracle-plugin-rs
This package provides a pleasant API for writing plugins for [miracle-wm](https://github.com/miracle-wm-org/miracle-wm).
Miracle's plugin system uses WebAssembly

## Setup
```sh
sudo apt-get install -y libmircore-dev clang libclang-dev
rustup target add wasm32-wasip1
```

## Building
```sh
cargo build --target wasm32-wasip1 --release
```

## Usage
First, make sure that you have the Mir and Rust dependencies installed:
```sh
sudo apt-get install -y libmircore-dev clang libclang-dev
rustup target add wasm32-wasip1
```

Next, add `miracle-plugin` as a dependency of your crate:
```sh
cargo add miracle-plugin
```

Then, author your plugin (the following is a toy example):
```rust
// lib.rs

// First, declare your plugin.
struct MyVeryCoolMiraclePlugin;

impl Plugin for MyVeryCoolMiraclePlugin {
    fn place_new_window(&mut self, info: &WindowInfo) -> Option<Placement> {
        if info.window_type != WindowType::Normal && info.window_type != WindowType::Freestyle {
            return None;
        }

        if info.state == WindowState::Attached {
            return None;
        }

        Some(Placement {
            strategy: WindowManagementStrategy::Freestyle,
            freestyle: FreestylePlacement {
                top_left: Point::new(100, 100),
                depth_layer: DepthLayer::Application,
                workspace: None,
                size: Size::new(500, 500),
                transform: Mat4::IDENTITY,
                alpha: 1.0,
                movable: false,
                resizable: false
            },
            ..Default::default()
        })
    }
    
    // And many more hooks...!
}

// Then, register it with Miracle.
miracle_plugin!(Miri);
```

Next, build your plugin with:
```sh
cargo build --target wasm32-wasip1
# or
cargo build --target wasm32-wasip1 --release
```

Finally, modify your Miracle config to load the plugin:

```yaml
# ~/.config/miracle-wm/config.yaml

plugins:
    - path: /path/to/myplugin.wasm
```
