//! Rust bindings for the miracle-wm plugin API.
//!
//! This crate provides auto-generated bindings from `plugin.h` using bindgen.

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
