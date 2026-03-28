use crate::animation::{AnimationFrameData, AnimationFrameResult};
use crate::config::Configuration;
use crate::host::*;
use crate::input::{KeyboardEvent, PointerEvent};
use crate::output::*;
use crate::placement::Placement;
use crate::window::{PluginWindow, WindowInfo};
use crate::workspace::*;

unsafe extern "C" {
    fn miracle_get_plugin_handle() -> u32;
}

/// Retrieve the raw JSON string of userdata configured for this plugin in the YAML config.
///
/// Returns `None` if no userdata was provided. The JSON object contains all keys from
/// the plugin's config entry except `path`.
pub fn get_userdata_json() -> Option<String> {
    let handle = unsafe { miracle_get_plugin_handle() };
    let mut buf = vec![0u8; 4096];
    loop {
        let result = unsafe {
            crate::host::miracle_get_plugin_userdata(
                handle,
                buf.as_mut_ptr() as i32,
                buf.len() as i32,
            )
        };
        if result == 0 {
            return None;
        } else if result == -1 {
            buf.resize(buf.len() * 2, 0);
        } else {
            return std::str::from_utf8(&buf[..result as usize])
                .ok()
                .map(|s| s.to_owned());
        }
    }
}

/// The core plugin trait.
///
/// Implement this trait to respond to compositor events. All methods have default no-op
/// implementations, so you only need to override the hooks you care about.
///
/// Register your implementation with the [`miracle_plugin!`] macro:
///
/// ```rust,ignore
/// #[derive(Default)]
/// struct MyPlugin;
/// impl Plugin for MyPlugin {}
/// miracle_plugin::miracle_plugin!(MyPlugin);
/// ```
pub trait Plugin {
    /// Handles the window opening animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn window_open_animation(
        &mut self,
        _data: &AnimationFrameData,
        _window: &WindowInfo,
    ) -> Option<AnimationFrameResult> {
        None
    }

    /// Handles the window closing animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn window_close_animation(
        &mut self,
        _data: &AnimationFrameData,
        _window: &WindowInfo,
    ) -> Option<AnimationFrameResult> {
        None
    }

    /// Handles the window movement animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn window_move_animation(
        &mut self,
        _data: &AnimationFrameData,
        _window: &WindowInfo,
    ) -> Option<AnimationFrameResult> {
        None
    }

    /// Handles the workspace switching animation.
    ///
    /// If None is returned, the animation is not handled by this plugin.
    fn workspace_switch_animation(
        &mut self,
        _data: &AnimationFrameData,
        _workspace: &Workspace,
    ) -> Option<AnimationFrameResult> {
        None
    }

    /// Dictate the placement of a newly opened window.
    ///
    /// Return a [`Placement`] to override where and how the window is placed.
    /// Return `None` to let the compositor handle placement normally.
    fn place_new_window(&mut self, _info: &WindowInfo) -> Option<Placement> {
        None
    }

    /// Called when a window is about to be deleted.
    ///
    /// The window info is still valid at this point (the window has not yet
    /// been removed from the compositor).
    fn window_deleted(&mut self, _info: &WindowInfo) {}

    /// Called when a window gains focus.
    fn window_focused(&mut self, _info: &WindowInfo) {}

    /// Called when a window loses focus.
    fn window_unfocused(&mut self, _info: &WindowInfo) {}

    /// Called when a workspace is created.
    fn workspace_created(&mut self, _workspace: &Workspace) {}

    /// Called when a workspace is removed.
    fn workspace_removed(&mut self, _workspace: &Workspace) {}

    /// Called when a workspace gains focus.
    ///
    /// `previous_id` is the internal ID of the previously focused workspace, if any.
    fn workspace_focused(&mut self, _previous_id: Option<u64>, _current: &Workspace) {}

    /// Called when a workspace's area (geometry) changes.
    fn workspace_area_changed(&mut self, _workspace: &Workspace) {}

    /// Called when a window's workspace has changed.
    ///
    /// This fires whenever a window is moved to a different workspace,
    /// whether initiated by the user, a command, or a plugin.
    fn window_workspace_changed(&mut self, _info: &WindowInfo, _workspace: &Workspace) {}

    /// Called on every config reload. Return a [`Configuration`] with the fields
    /// this plugin wants to override, or `None` to leave the compositor's config unchanged.
    ///
    /// Only fields that are `Some(...)` are applied; `None` fields are ignored.
    /// The `plugins` key cannot be set by plugins and is intentionally absent from
    /// [`Configuration`].
    ///
    /// Results from all loaded plugins are merged before being applied to the
    /// file-based configuration (last-loaded plugin wins on field conflicts).
    fn configure(&mut self) -> Option<Configuration> {
        None
    }

    /// Handle a keyboard event.
    ///
    /// If the plugin returns `false`, the event is propagated to the next
    /// handler in Miracle. If the plugin returns `true`, then the event is
    /// consumed by the plugin.
    fn handle_keyboard_input(&mut self, _event: KeyboardEvent) -> bool {
        false
    }

    /// Handle a pointer event.
    ///
    /// If the plugin returns `false`, the event is propagated to the next
    /// handler in Miracle. If the plugin returns `true`, then the event is
    /// consumed by the plugin.
    fn handle_pointer_event(&mut self, _event: PointerEvent) -> bool {
        false
    }
}

/// Lists the windows that are managed by this plugin.
///
/// A window that is managed by this plugin had to have been placed
/// via a freestyle placement strategy, otherwise the tiling manager
/// or the system is handling it independently.
///
/// Each returned [`PluginWindow`] wraps a [`WindowInfo`] (accessible via `Deref`) and
/// additionally exposes setter methods for mutating the window's state, workspace,
/// size, transform, and alpha.
pub fn managed_windows() -> Vec<PluginWindow> {
    let handle = unsafe { miracle_get_plugin_handle() };
    let count = unsafe { miracle_num_managed_windows(handle) };

    (0..count)
        .filter_map(|i| {
            const NAME_BUF_LEN: usize = 256;
            let mut window_info =
                std::mem::MaybeUninit::<crate::bindings::miracle_window_info_t>::uninit();
            let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

            unsafe {
                let result = miracle_get_managed_window_at(
                    handle,
                    i,
                    window_info.as_mut_ptr() as i32,
                    name_buf.as_mut_ptr() as i32,
                    NAME_BUF_LEN as i32,
                );

                if result != 0 {
                    return None;
                }

                let window_info = window_info.assume_init();
                let name_len = name_buf
                    .iter()
                    .position(|&c| c == 0)
                    .unwrap_or(NAME_BUF_LEN);
                let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

                Some(PluginWindow::from_window_info(
                    WindowInfo::from_c_with_name(&window_info, name),
                ))
            }
        })
        .collect()
}

/// Get the number of outputs.
pub fn num_outputs() -> u32 {
    unsafe { miracle_num_outputs() }
}

/// Get an output by index.
///
/// Returns `None` if the index is out of bounds or if the call fails.
pub fn get_output_at(index: u32) -> Option<Output> {
    if index >= num_outputs() {
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
pub fn get_outputs() -> Vec<Output> {
    let count = num_outputs();
    (0..count).filter_map(get_output_at).collect()
}

/// Get the currently active workspace on the focused output.
///
/// Returns `None` if there is no focused output or no active workspace.
pub fn get_active_workspace() -> Option<Workspace> {
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
pub fn request_workspace(
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

/// Queue a custom per-frame animation with a callback.
///
/// `callback` receives `(animation_id, dt, elapsed_seconds)` every frame.
/// The compositor automatically removes the animation after `duration_seconds`.
///
/// `dt` and `elapsed_seconds` are floats in seconds.
///
/// Returns the host-generated animation ID on success, or `None` on error.
pub fn queue_custom_animation<F>(callback: F, duration_seconds: f32) -> Option<u32>
where
    F: FnMut(u32, f32, f32) + 'static,
{
    let handle = unsafe { miracle_get_plugin_handle() };
    let mut animation_id: u32 = 0;
    let mut dur = duration_seconds;
    let result = unsafe {
        crate::host::miracle_queue_custom_animation(
            handle as i32,
            &mut animation_id as *mut u32 as i32,
            &mut dur as *mut f32 as i32,
        )
    };
    if result == 0 {
        custom_anim_callbacks().insert(animation_id, (Box::new(callback), duration_seconds));
        Some(animation_id)
    } else {
        None
    }
}

static mut _CUSTOM_ANIM_CALLBACKS: Option<
    std::collections::HashMap<u32, (Box<dyn FnMut(u32, f32, f32)>, f32)>,
> = None;

/// Returns the global custom-animation callback registry.
///
/// # Safety
/// Only safe in a single-threaded WASM context (which is always the case for miracle plugins).
#[doc(hidden)]
pub fn custom_anim_callbacks()
-> &'static mut std::collections::HashMap<u32, (Box<dyn FnMut(u32, f32, f32)>, f32)> {
    unsafe {
        if (*std::ptr::addr_of!(_CUSTOM_ANIM_CALLBACKS)).is_none() {
            _CUSTOM_ANIM_CALLBACKS = Some(std::collections::HashMap::new());
        }
        (*std::ptr::addr_of_mut!(_CUSTOM_ANIM_CALLBACKS))
            .as_mut()
            .unwrap()
    }
}

/// Registers a type as a miracle-wm plugin.
///
/// The type must implement [`Default`] and [`Plugin`]. This macro generates all of the
/// required WASM export functions (`init`, `animate`, `place_new_window`, etc.) that the
/// compositor calls at runtime.
///
/// # Example
///
/// ```rust,ignore
/// #[derive(Default)]
/// struct MyPlugin;
/// impl Plugin for MyPlugin {}
/// miracle_plugin!(MyPlugin);
/// ```
#[macro_export]
macro_rules! miracle_plugin {
    ($plugin_type:ty) => {
        static mut _MIRACLE_PLUGIN: Option<$plugin_type> = None;
        static mut _MIRACLE_PLUGIN_HANDLE: u32 = 0;

        #[unsafe(no_mangle)]
        pub extern "C" fn miracle_get_plugin_handle() -> u32 {
            unsafe { _MIRACLE_PLUGIN_HANDLE }
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn init(handle: i32) {
            unsafe {
                _MIRACLE_PLUGIN_HANDLE = handle as u32;
                _MIRACLE_PLUGIN = Some(<$plugin_type>::default());
            }
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn animate(data_ptr: i32, result_ptr: i32) -> i32 {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return 0,
                }
            };

            let c_data = unsafe {
                &*(data_ptr as *const $crate::bindings::miracle_plugin_animation_frame_data_t)
            };
            let data: $crate::animation::AnimationFrameData = (*c_data).into();

            let extract_window = || {
                let bytes = unsafe {
                    core::slice::from_raw_parts(c_data.window_name.as_ptr() as *const u8, 256)
                };
                let len = bytes.iter().position(|&b| b == 0).unwrap_or(256);
                let name = String::from_utf8_lossy(&bytes[..len]).into_owned();
                unsafe { $crate::window::WindowInfo::from_c_with_name(&c_data.window_info, name) }
            };

            let extract_workspace = || {
                let bytes = unsafe {
                    core::slice::from_raw_parts(c_data.workspace_name.as_ptr() as *const u8, 256)
                };
                let len = bytes.iter().position(|&b| b == 0).unwrap_or(256);
                let name = String::from_utf8_lossy(&bytes[..len]).into_owned();
                unsafe { $crate::workspace::Workspace::from_c_with_name(&c_data.workspace, name) }
            };

            let write_result = |result: $crate::animation::AnimationFrameResult| {
                let c_result: $crate::bindings::miracle_plugin_animation_frame_result_t =
                    result.into();
                unsafe {
                    let out = &mut *(result_ptr
                        as *mut $crate::bindings::miracle_plugin_animation_frame_result_t);
                    *out = c_result;
                }
            };

            match c_data.type_ {
                $crate::bindings::miracle_animation_type_miracle_animation_type_window_open => {
                    let window = extract_window();
                    match plugin.window_open_animation(&data, &window) {
                        Some(result) => { write_result(result); 1 }
                        None => 0,
                    }
                },
                $crate::bindings::miracle_animation_type_miracle_animation_type_window_close => {
                        let window = extract_window();
                    match plugin.window_close_animation(&data, &window) {
                        Some(result) => { write_result(result); 1 }
                        None => 0,
                    }
                },
                $crate::bindings::miracle_animation_type_miracle_animation_type_window_move => {
                    let window = extract_window();
                    match plugin.window_move_animation(&data, &window) {
                        Some(result) => { write_result(result); 1 }
                        None => 0,
                    }
                },
                $crate::bindings::miracle_animation_type_miracle_animation_type_workspace_switch => {
                    let workspace = extract_workspace();
                    match plugin.workspace_switch_animation(&data, &workspace) {
                        Some(result) => { write_result(result); 1 }
                        None => 0,
                    }
                },
                _ => 0
            }

        }

        #[unsafe(no_mangle)]
        pub extern "C" fn custom_animate(data_ptr: i32) -> i32 {
            let raw = unsafe {
                &*(data_ptr as *const $crate::animation::RawCustomAnimationData)
            };

            let callbacks = $crate::plugin::custom_anim_callbacks();
            let done = if let Some((cb, dur)) = callbacks.get_mut(&raw.animation_id) {
                cb(raw.animation_id, raw.dt, raw.elapsed_seconds);
                raw.elapsed_seconds >= *dur
            } else {
                false
            };
            if done {
                callbacks.remove(&raw.animation_id);
            }

            // Return value is ignored by the host; kept for WASM ABI compatibility.
            0
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn place_new_window(
            window_info_ptr: i32,
            result_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) -> i32 {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return 0,
                }
            };

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

            match plugin.place_new_window(&info) {
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

        #[unsafe(no_mangle)]
        pub extern "C" fn window_deleted(
            window_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

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

            plugin.window_deleted(&info);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn window_focused(
            window_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

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

            plugin.window_focused(&info);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn window_unfocused(
            window_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

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

            plugin.window_unfocused(&info);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn workspace_created(
            workspace_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

            let c_ws = unsafe {
                &*(workspace_info_ptr as *const $crate::bindings::miracle_workspace_t)
            };

            let name = if name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(name_ptr as *const u8, name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let ws = unsafe { $crate::workspace::Workspace::from_c_with_name(c_ws, name) };
            plugin.workspace_created(&ws);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn workspace_removed(
            workspace_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

            let c_ws = unsafe {
                &*(workspace_info_ptr as *const $crate::bindings::miracle_workspace_t)
            };

            let name = if name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(name_ptr as *const u8, name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let ws = unsafe { $crate::workspace::Workspace::from_c_with_name(c_ws, name) };
            plugin.workspace_removed(&ws);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn workspace_focused(
            workspace_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
            has_previous: i32,
            previous_id: i64,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

            let c_ws = unsafe {
                &*(workspace_info_ptr as *const $crate::bindings::miracle_workspace_t)
            };

            let name = if name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(name_ptr as *const u8, name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let ws = unsafe { $crate::workspace::Workspace::from_c_with_name(c_ws, name) };
            let prev = if has_previous != 0 { Some(previous_id as u64) } else { None };
            plugin.workspace_focused(prev, &ws);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn workspace_area_changed(
            workspace_info_ptr: i32,
            name_ptr: i32,
            name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

            let c_ws = unsafe {
                &*(workspace_info_ptr as *const $crate::bindings::miracle_workspace_t)
            };

            let name = if name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(name_ptr as *const u8, name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let ws = unsafe { $crate::workspace::Workspace::from_c_with_name(c_ws, name) };
            plugin.workspace_area_changed(&ws);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn window_workspace_changed(
            window_info_ptr: i32,
            window_name_ptr: i32,
            window_name_len: i32,
            workspace_info_ptr: i32,
            workspace_name_ptr: i32,
            workspace_name_len: i32,
        ) {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return,
                }
            };

            let c_info = unsafe {
                &*(window_info_ptr as *const $crate::bindings::miracle_window_info_t)
            };

            let window_name = if window_name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(window_name_ptr as *const u8, window_name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let info = unsafe { $crate::window::WindowInfo::from_c_with_name(c_info, window_name) };

            let c_ws = unsafe {
                &*(workspace_info_ptr as *const $crate::bindings::miracle_workspace_t)
            };

            let workspace_name = if workspace_name_len > 0 {
                let name_bytes = unsafe {
                    core::slice::from_raw_parts(workspace_name_ptr as *const u8, workspace_name_len as usize)
                };
                String::from_utf8_lossy(name_bytes).into_owned()
            } else {
                String::new()
            };

            let ws = unsafe { $crate::workspace::Workspace::from_c_with_name(c_ws, workspace_name) };
            plugin.window_workspace_changed(&info, &ws);
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn configure(buf_ptr: i32, buf_len: i32) -> i32 {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return 0,
                }
            };
            $crate::__private::run_configure(plugin, buf_ptr, buf_len)
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn handle_keyboard_input(event_ptr: i32) -> i32 {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return 0,
                }
            };

            let c_event = unsafe {
                &*(event_ptr as *const $crate::bindings::miracle_keyboard_event_t)
            };

            let event = $crate::input::KeyboardEvent {
                action: $crate::input::KeyboardAction::try_from(c_event.action)
                    .unwrap_or_default(),
                keysym: c_event.keysym,
                scan_code: c_event.scan_code,
                modifiers: $crate::input::InputEventModifiers::from(c_event.modifiers),
            };

            if plugin.handle_keyboard_input(event) { 1 } else { 0 }
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn handle_pointer_event(event_ptr: i32) -> i32 {
            let plugin = unsafe {
                match _MIRACLE_PLUGIN.as_mut() {
                    Some(p) => p,
                    None => return 0,
                }
            };

            let c_event = unsafe {
                &*(event_ptr as *const $crate::bindings::miracle_pointer_event_t)
            };

            let event = $crate::input::PointerEvent {
                x: c_event.x,
                y: c_event.y,
                action: $crate::input::PointerAction::try_from(c_event.action)
                    .unwrap_or_default(),
                modifiers: $crate::input::InputEventModifiers::from(c_event.modifiers),
                buttons: $crate::input::PointerButtons::from(c_event.buttons),
            };

            if plugin.handle_pointer_event(event) { 1 } else { 0 }
        }
    };
}
