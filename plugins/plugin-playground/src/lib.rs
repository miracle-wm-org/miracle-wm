use miracle_plugin_rs::{
    animation::{AnimationFrameData, AnimationFrameResult},
    miracle_plugin,
    plugin::Plugin,
};

#[derive(Default)]
struct PluginPlayground;

impl Plugin for PluginPlayground {
    fn window_open_animation(&self, data: &AnimationFrameData) -> Option<AnimationFrameResult> {
        let progress = (data.runtime_seconds / data.duration_seconds).clamp(0.0, 1.0);

        let eased = ease_in_cubic(progress);
        let opacity = data.opacity_start + (data.opacity_end - data.opacity_start) * eased;

        Some(AnimationFrameResult {
            completed: false,
            area: Some(data.destination),
            transform: None,
            opacity: Some(opacity),
        })
    }
}

miracle_plugin!(PluginPlayground);

fn ease_in_cubic(t: f32) -> f32 {
    t * t * t
}
