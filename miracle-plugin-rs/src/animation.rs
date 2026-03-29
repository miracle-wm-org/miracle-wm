use crate::bindings;
use crate::core::{Rect, mat4_from_f32_array, mat4_to_f32_array};
use glam::Mat4;

#[doc(hidden)]
#[repr(C)]
pub struct RawCustomAnimationData {
    pub animation_id: u32,
    pub dt: f32,
    pub elapsed_seconds: f32,
}

/// Data passed to animation hook methods describing the current frame.
pub struct AnimationFrameData {
    /// Elapsed time since the animation started, in seconds.
    pub runtime_seconds: f32,
    /// The starting rectangle of the window (position and size at animation start).
    pub origin: Rect,
    /// The target rectangle of the window (position and size at animation end).
    pub destination: Rect,
    /// The starting opacity of the window.
    pub opacity_start: f32,
    /// The target opacity of the window.
    pub opacity_end: f32,
}

impl From<bindings::miracle_plugin_animation_frame_data_t> for AnimationFrameData {
    fn from(value: bindings::miracle_plugin_animation_frame_data_t) -> Self {
        Self {
            runtime_seconds: value.runtime_seconds,
            origin: Rect::from_array(value.origin),
            destination: Rect::from_array(value.destination),
            opacity_start: value.opacity_start,
            opacity_end: value.opacity_end,
        }
    }
}

/// Returned from animation hooks to describe the frame's visual state.
pub struct AnimationFrameResult {
    /// Set to `true` to signal that the animation is finished.
    pub completed: bool,
    /// Override the window's rectangle for this frame. `None` leaves it unchanged.
    ///
    /// When also setting `clip_area`, this should carry the window's **final target
    /// size** so the client receives the correct configure event immediately.
    pub area: Option<Rect>,
    /// Override the window's transform matrix for this frame. `None` leaves it unchanged.
    pub transform: Option<Mat4>,
    /// Override the window's opacity for this frame. `None` leaves it unchanged.
    pub opacity: Option<f32>,
    /// Set the scissor-clip rectangle for this frame independently from `area`.
    ///
    /// When set, this rectangle is used as the compositor scissor region rather
    /// than `area`, allowing the window surface to be revealed gradually while
    /// `area` carries the final target geometry for the Wayland configure event.
    /// This avoids blank space during resize animations.
    pub clip_area: Option<Rect>,
}

impl From<AnimationFrameResult> for bindings::miracle_plugin_animation_frame_result_t {
    fn from(value: AnimationFrameResult) -> Self {
        Self {
            completed: value.completed as i32,
            has_area: value.area.is_some() as i32,
            area: value.area.map(|r| r.to_array()).unwrap_or_default(),
            has_transform: value.transform.is_some() as i32,
            transform: value.transform.map(mat4_to_f32_array).unwrap_or_default(),
            has_opacity: value.opacity.is_some() as i32,
            opacity: value.opacity.unwrap_or_default(),
            has_clip_area: value.clip_area.is_some() as i32,
            clip_area: value.clip_area.map(|r| r.to_array()).unwrap_or_default(),
        }
    }
}

impl From<bindings::miracle_plugin_animation_frame_result_t> for AnimationFrameResult {
    fn from(value: bindings::miracle_plugin_animation_frame_result_t) -> Self {
        Self {
            completed: value.completed != 0,
            area: if value.has_area != 0 {
                Some(Rect::from_array(value.area))
            } else {
                None
            },
            transform: if value.has_transform != 0 {
                Some(mat4_from_f32_array(value.transform))
            } else {
                None
            },
            opacity: if value.has_opacity != 0 {
                Some(value.opacity)
            } else {
                None
            },
            clip_area: if value.has_clip_area != 0 {
                Some(Rect::from_array(value.clip_area))
            } else {
                None
            },
        }
    }
}
