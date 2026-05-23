#![doc(html_root_url = "https://docs.miracle-wm.org/miracle_plugin/")]
//! # miracle-plugin
//!
//! A Rust SDK for writing [miracle-wm](https://github.com/miracle-wm-org/miracle-wm) plugins.
//!
//! Miracle's plugin system runs each plugin as a WebAssembly module. This crate provides
//! idiomatic Rust types and traits that map to the compositor's C ABI, so you can write
//! plugins without touching raw FFI.
//!
//! ## Quick start
//!
//! Create a new library crate and add `miracle-plugin` as a dependency:
//!
//! ```bash
//! cargo new --lib my-plugin
//! cd my-plugin
//! cargo add miracle-plugin
//! ```
//!
//! Set the crate type to `cdylib` in `Cargo.toml`:
//!
//! ```toml
//! # Cargo.toml
//! [lib]
//! crate-type = ["cdylib"]
//! ```
//!
//! If you would like to configure from your plugin, enable the configure feature, e.g.:
//! ```toml
//! # Cargo.toml
//! [dependencies]
//! miracle-plugin = { version = "0.0.10", features = ["configure"]}
//! ```
//!
//! Add the `wasm32-wasip1` target if you haven't already:
//!
//! ```bash
//! rustup target add wasm32-wasip1
//! ```
//!
//! Implement the [`plugin::Plugin`] trait and register your type with the
//! [`miracle_plugin!`] macro:
//!
//! ```rust,ignore
//! use miracle_plugin::plugin::Plugin;
//!
//! #[derive(Default)]
//! struct MyPlugin;
//!
//! impl Plugin for MyPlugin {
//!     // Your plugin implementation here.
//! }
//!
//! miracle_plugin::miracle_plugin!(MyPlugin);
//! ```
//!
//! Build the plugin as a `.wasm` module:
//!
//! ```bash
//! cargo build --target wasm32-wasip1 --release
//! ```
//!
//! The compiled `.wasm` file will be at
//! `target/wasm32-wasip1/release/my_plugin.wasm`.
//!
//! ## Modules
//!
//! | Module | Contents |
//! |---|---|
//! | [`plugin`] | [`plugin::Plugin`] trait, [`miracle_plugin!`] macro, helper functions |
//! | [`window`] | [`window::WindowInfo`], window-state enums |
//! | [`placement`] | [`placement::Placement`] |
//! | [`animation`] | [`animation::AnimationFrameData`], [`animation::AnimationFrameResult`] |
//! | [`input`] | [`input::KeyboardEvent`], [`input::PointerEvent`], modifier/button flags |
//! | [`core`] | Geometric primitives: [`core::Rect`], [`core::Point`], [`core::Size`], [`core::Rectangle`] |
//! | [`container`] | [`container::Container`], [`container::ContainerType`], [`container::LayoutScheme`] |
//! | [`workspace`] | [`workspace::Workspace`] |
//! | [`output`] | [`output::Output`] |
//! | [`application`] | [`application::ApplicationInfo`] |
//! | [`config`] | [`config::Configuration`] and all supporting types ([`config::Modifier`], [`config::Key`], [`config::BindingAction`], …) |

pub mod animation;
pub mod application;
#[doc(hidden)]
#[allow(warnings)]
pub mod bindings;
pub mod config;
pub mod container;
pub mod core;
#[doc(hidden)]
pub mod host;
pub mod input;
pub mod output;
pub mod placement;
pub mod plugin;
pub mod window;
pub mod workspace;

#[doc(hidden)]
pub mod __private {
    #[cfg(feature = "configure")]
    pub use serde_json;

    /// Called by the `miracle_plugin!` macro to serialize the plugin's configuration.
    ///
    /// Keeping the `#[cfg(feature = "configure")]` here (in the defining crate) means
    /// the feature flag is resolved against miracle-plugin's own Cargo features, not
    /// against the calling plugin crate's features — which avoids the
    /// `unexpected_cfgs` warning that would fire if `#[cfg(feature = "configure")]`
    /// appeared directly inside the macro body.
    #[cfg(feature = "configure")]
    pub fn run_configure<P: crate::plugin::Plugin>(
        plugin: &mut P,
        buf_ptr: i32,
        buf_len: i32,
    ) -> i32 {
        match plugin.configure() {
            None => 0,
            Some(config_data) => {
                let json = match serde_json::to_string(&config_data) {
                    Ok(s) => s,
                    Err(_) => return 0,
                };
                let bytes = json.as_bytes();
                if bytes.len() > buf_len as usize {
                    return -1;
                }
                unsafe {
                    let buf = core::slice::from_raw_parts_mut(buf_ptr as *mut u8, buf_len as usize);
                    buf[..bytes.len()].copy_from_slice(bytes);
                }
                bytes.len() as i32
            }
        }
    }

    #[cfg(not(feature = "configure"))]
    pub fn run_configure<P: crate::plugin::Plugin>(
        _plugin: &mut P,
        _buf_ptr: i32,
        _buf_len: i32,
    ) -> i32 {
        0
    }
}

pub use config::{BindingAction, Key, Modifier};
pub use plugin::{
    get_active_workspace, get_output_at, get_outputs, get_userdata_json, managed_windows,
    num_outputs, queue_custom_animation, request_workspace,
};
