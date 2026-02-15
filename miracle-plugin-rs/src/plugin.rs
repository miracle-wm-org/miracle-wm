use super::animation::{AnimationFrameData, AnimationFrameResult};

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
    };
}
