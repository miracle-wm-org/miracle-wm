use crate::placement::Placement;
use crate::window::WindowInfo;

use super::animation::{AnimationFrameData, AnimationFrameResult};
use super::host::*;
use super::output::*;
use super::workspace::*;

pub trait Plugin {
    /// Handles the window opening animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn window_open_animation(&self, data: &AnimationFrameData) -> Option<AnimationFrameResult> {
        None
    }

    /// Handles the window closing animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn window_close_animation(&self, data: &AnimationFrameData) -> Option<AnimationFrameResult> {
        None
    }

    /// Handles the window movement animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn window_move_animation(&self, data: &AnimationFrameData) -> Option<AnimationFrameResult> {
        None
    }

    /// Handles the workspace switching animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn workspace_switch_animation(
        &self,
        data: &AnimationFrameData,
    ) -> Option<AnimationFrameResult> {
        None
    }

    /// Place a new window.
    ///
    // If None is returned, the placement is not handled by this plugin.
    fn place_new_window(info: WindowInfo) -> Option<Placement> {
        None
    }

    /// Get the number of outputs.
    fn num_outputs() -> u32 {
        unsafe { miracle_num_outputs() }
    }

    /// Get an output by index.
    ///
    /// Returns `None` if the index is out of bounds or if the call fails.
    fn get_output_at(index: u32) -> Option<Output> {
        if index >= Self::num_outputs() {
            return None;
        }

        const NAME_BUF_LEN: usize = 256;
        let mut output = std::mem::MaybeUninit::<crate::bindings::miracle_output_t>::uninit();
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let result = miracle_get_output_at(
                index,
                output.as_mut_ptr() as i32,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if result != 0 {
                return None;
            }

            let output = output.assume_init();

            // Find the null terminator to get the actual string length
            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(Output::from_c_with_name(&output, name))
        }
    }

    /// Get all outputs.
    fn get_outputs() -> Vec<Output> {
        let count = Self::num_outputs();
        (0..count).filter_map(|i| Self::get_output_at(i)).collect()
    }

    /// Get the currently active workspace on the focused output.
    ///
    /// Returns `None` if there is no focused output or no active workspace.
    fn get_active_workspace() -> Option<Workspace> {
        const NAME_BUF_LEN: usize = 256;
        let mut workspace = std::mem::MaybeUninit::<crate::bindings::miracle_workspace_t>::uninit();
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let result = miracle_get_active_workspace(
                workspace.as_mut_ptr() as i32,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if result != 0 {
                return None;
            }

            let workspace = workspace.assume_init();
            if workspace.is_set == 0 {
                return None;
            }

            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(Workspace::from_c_with_name(&workspace, name))
        }
    }

    /// Request a workspace by optional number and/or name.
    ///
    /// If a workspace with the given number or name already exists, it is returned.
    /// Otherwise, a new workspace is created on the focused output.
    ///
    /// If `focus` is true, the workspace will be focused after creation/lookup.
    ///
    /// Returns `None` if the workspace could not be created.
    fn request_workspace(
        number: Option<u32>,
        name: Option<&str>,
        focus: bool,
    ) -> Option<Workspace> {
        const NAME_BUF_LEN: usize = 256;
        let mut workspace = std::mem::MaybeUninit::<crate::bindings::miracle_workspace_t>::uninit();
        let mut out_name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        let has_number: i32 = if number.is_some() { 1 } else { 0 };
        let number_val: i32 = number.unwrap_or(0) as i32;

        let name_ptr: i32 = match name {
            Some(s) => s.as_ptr() as i32,
            None => 0,
        };
        let name_len: i32 = match name {
            Some(s) => s.len() as i32,
            None => 0,
        };

        unsafe {
            let result = miracle_request_workspace(
                has_number,
                number_val,
                name_ptr,
                name_len,
                workspace.as_mut_ptr() as i32,
                out_name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
                if focus { 1 } else { 0 },
            );

            if result != 0 {
                return None;
            }

            let workspace = workspace.assume_init();
            if workspace.is_set == 0 {
                return None;
            }

            let out_name_len = out_name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let ws_name = String::from_utf8_lossy(&out_name_buf[..out_name_len]).into_owned();

            Some(Workspace::from_c_with_name(&workspace, ws_name))
        }
    }
}

#[macro_export]
macro_rules! miracle_plugin {
    ($plugin_type:ty) => {
        static mut _MIRACLE_PLUGIN: Option<$plugin_type> = None;

        #[unsafe(no_mangle)]
        pub extern "C" fn init() {
            unsafe {
                _MIRACLE_PLUGIN = Some(<$plugin_type>::default());
            }
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn animate(data_ptr: i32, result_ptr: i32) -> i32 {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_ref() {
                    Some(p) => p,
                    None => return 0,
                }
            };

            let c_data = unsafe {
                &*(data_ptr as *const $crate::bindings::miracle_plugin_animation_frame_data_t)
            };
            let data: AnimationFrameData = (*c_data).into();

            match c_data.type_ {
                $crate::bindings::miracle_animation_type_miracle_animation_type_window_open => {
                    match plugin.window_open_animation(&data) {
                        Some(result) => {
                            let c_result: $crate::bindings::miracle_plugin_animation_frame_result_t =
                                result.into();
                            unsafe {
                                let out = &mut *(result_ptr
                                    as *mut $crate::bindings::miracle_plugin_animation_frame_result_t);
                                *out = c_result;
                            }
                            return 1;
                        }
                        None => 0,
                    }
                },
                $crate::bindings::miracle_animation_type_miracle_animation_type_window_close => {
                    match plugin.window_close_animation(&data) {
                        Some(result) => {
                            let c_result: $crate::bindings::miracle_plugin_animation_frame_result_t =
                                result.into();
                            unsafe {
                                let out = &mut *(result_ptr
                                    as *mut $crate::bindings::miracle_plugin_animation_frame_result_t);
                                *out = c_result;
                            }
                            return 1;
                        }
                        None => 0,
                    }
                },
                $crate::bindings::miracle_animation_type_miracle_animation_type_window_move => {
                    match plugin.window_move_animation(&data) {
                        Some(result) => {
                            let c_result: $crate::bindings::miracle_plugin_animation_frame_result_t =
                                result.into();
                            unsafe {
                                let out = &mut *(result_ptr
                                    as *mut $crate::bindings::miracle_plugin_animation_frame_result_t);
                                *out = c_result;
                            }
                            return 1;
                        }
                        None => 0,
                    }
                },
                $crate::bindings::miracle_animation_type_miracle_animation_type_workspace_switch => {
                    match plugin.workspace_switch_animation(&data) {
                        Some(result) => {
                            let c_result: $crate::bindings::miracle_plugin_animation_frame_result_t =
                                result.into();
                            unsafe {
                                let out = &mut *(result_ptr
                                    as *mut $crate::bindings::miracle_plugin_animation_frame_result_t);
                                *out = c_result;
                            }
                            return 1;
                        }
                        None => 0,
                    }
                },
                _ => 0
            }

        }

        #[unsafe(no_mangle)]
        pub extern "C" fn place_new_window(
            window_info_ptr: i32,
            result_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) -> i32 {
            let c_info = unsafe {
                &*(window_info_ptr as *const $crate::bindings::miracle_window_info_t)
            };

            let name = if name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(name_ptr as *const u8, name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let info = unsafe { $crate::window::WindowInfo::from_c_with_name(c_info, name) };

            match <$plugin_type as $crate::plugin::Plugin>::place_new_window(info) {
                Some(placement) => {
                    let c_placement: $crate::bindings::miracle_placement_t = placement.into();
                    unsafe {
                        let out = &mut *(result_ptr as *mut $crate::bindings::miracle_placement_t);
                        *out = c_placement;
                    }
                    1
                }
                None => 0,
            }
        }
    };
}
