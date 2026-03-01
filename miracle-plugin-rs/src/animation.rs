use crate::bindings;
use crate::core::{Rect, mat4_from_f32_array, mat4_to_f32_array};
use glam::Mat4;

pub struct AnimationFrameData {
    pub runtime_seconds: f32,
    pub duration_seconds: f32,
    pub origin: Rect,
    pub destination: Rect,
    pub opacity_start: f32,
    pub opacity_end: f32,
}

impl From<bindings::miracle_plugin_animation_frame_data_t> for AnimationFrameData {
    fn from(value: bindings::miracle_plugin_animation_frame_data_t) -> Self {
        Self {
            runtime_seconds: value.runtime_seconds,
            duration_seconds: value.duration_seconds,
            origin: Rect::from_array(value.origin),
            destination: Rect::from_array(value.destination),
            opacity_start: value.opacity_start,
            opacity_end: value.opacity_end,
        }
    }
}

pub struct AnimationFrameResult {
    pub completed: bool,
    pub area: Option<Rect>,
    pub transform: Option<Mat4>,
    pub opacity: Option<f32>,
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
        }
    }
}
