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
//! Add the crate as a dependency and set the crate type to `cdylib`:
//!
//! ```toml
//! # Cargo.toml
//! [lib]
//! crate-type = ["cdylib"]
//!
//! [dependencies]
//! miracle-plugin = "0.0.1"
//! ```
//!
//! Implement the [`plugin::Plugin`] trait and register your type with the
//! [`miracle_plugin!`] macro:
//!
//! ```rust,ignore
//! use miracle_plugin::plugin::Plugin;
//! use miracle_plugin::window::WindowInfo;
//! use miracle_plugin::placement::Placement;
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
//! ## Modules
//!
//! | Module | Contents |
//! |---|---|
//! | [`plugin`] | [`plugin::Plugin`] trait, [`miracle_plugin!`] macro, helper functions |
//! | [`window`] | [`window::WindowInfo`], [`window::PluginWindow`], window-state enums |
//! | [`placement`] | [`placement::Placement`] and placement strategy types |
//! | [`animation`] | [`animation::AnimationFrameData`], [`animation::AnimationFrameResult`] |
//! | [`input`] | [`input::KeyboardEvent`], [`input::PointerEvent`], modifier/button flags |
//! | [`core`] | Geometric primitives: [`core::Rect`], [`core::Point`], [`core::Size`], [`core::Rectangle`] |
//! | [`container`] | [`container::Container`], [`container::ContainerType`], [`container::LayoutScheme`] |
//! | [`workspace`] | [`workspace::Workspace`] |
//! | [`output`] | [`output::Output`] |
//! | [`application`] | [`application::ApplicationInfo`] |

pub mod animation;
pub mod application;
#[doc(hidden)]
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

pub use plugin::{
    get_active_workspace, get_output_at, get_outputs, get_userdata_json, managed_windows,
    num_outputs, queue_custom_animation, request_workspace,
};
