use miracle_plugin_rs::{
    animation::{AnimationFrameData, AnimationFrameResult},
    core::{Point, Size},
    miracle_plugin,
    placement::{FreestylePlacement, Placement, WindowManagementStrategy},
    plugin::Plugin,
    window::{DepthLayer, WindowInfo},
};

#[derive(Default)]
struct PluginPlayground {
    num_windows: i32,
}

impl Plugin for PluginPlayground {
    fn place_new_window(&mut self, info: WindowInfo) -> Option<Placement> {
        let x = self.num_windows * 100;
        self.num_windows += 1;
        Some(Placement {
            strategy: WindowManagementStrategy::Freestyle,
            freestyle: FreestylePlacement {
                top_left: Point::new(x, 100),
                depth_layer: DepthLayer::Application,
                workspace: None,
                size: Size::new(800, 600),
            },
            ..Default::default()
        })
    }

    fn window_deleted(&mut self, info: WindowInfo) {
        self.num_windows -= 1;
    }

    fn window_open_animation(&mut self, data: &AnimationFrameData) -> Option<AnimationFrameResult> {
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
