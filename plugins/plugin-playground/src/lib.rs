use miracle_plugin_rs::{
    miracle_context_t, miracle_placement_t, miracle_plugin_animation_frame_data_t,
    miracle_plugin_animation_frame_result_t, miracle_point_t, miracle_window_info_t,
    MirDepthLayer_mir_depth_layer_application,
};

#[unsafe(no_mangle)]
pub extern "C" fn add_points(a: miracle_point_t, b: miracle_point_t) -> miracle_point_t {
    miracle_point_t {
        x: a.x + b.x,
        y: a.y + b.y,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn animate(
    data: miracle_plugin_animation_frame_data_t,
) -> miracle_plugin_animation_frame_result_t {
    let progress = data.runtime_seconds / data.duration_seconds;
    let opacity = data.opacity_start + (data.opacity_end - data.opacity_start) * progress;
    miracle_plugin_animation_frame_result_t {
        completed: 0,
        has_area: 1,
        area: [
            data.destination[0],
            data.destination[1],
            data.destination[2],
            data.destination[3],
        ],
        has_transform: 0,
        transform: [0.0; 16],
        has_opacity: 1,
        opacity,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn place_new_window(
    result: *mut miracle_placement_t,
    _context: *const miracle_context_t,
    _window_info: *const miracle_window_info_t,
) {
    unsafe {
        (*result).is_set = 1;
        (*result).top_left.x = 100;
        (*result).top_left.y = 100;
        (*result).size.w = 800;
        (*result).size.h = 600;
        (*result).depth_layer = MirDepthLayer_mir_depth_layer_application;
    }
}
