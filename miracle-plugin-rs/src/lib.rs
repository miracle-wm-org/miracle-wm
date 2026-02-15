//! Rust bindings for the miracle-wm plugin API.
//!
//! This crate provides auto-generated bindings from `plugin.h` using bindgen,
//! along with idiomatic Rust wrappers in the [`types`] module.
//!
//! # Usage
//!
//! For low-level C interop, use the raw bindings directly:
//! ```ignore
//! use miracle_plugin_rs::bindings;
//! ```
//!
//! For idiomatic Rust types with automatic conversion:
//! ```ignore
//! use miracle_plugin_rs::types::{Point, Size, WindowState};
//! ```

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub mod animation;
pub mod bindings;
pub mod core;
pub mod plugin;
pub mod types;

// Re-export commonly used items at the crate root
pub use types::*;
