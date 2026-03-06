//! The miracle-wm plugin API.
//!

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub mod animation;
pub mod application;
pub mod bindings;
pub mod container;
pub mod core;
pub mod host;
pub mod input;
pub mod output;
pub mod placement;
pub mod plugin;
pub mod window;
pub mod workspace;

pub use plugin::get_userdata_json;
