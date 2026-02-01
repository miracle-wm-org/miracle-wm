use miracle_plugin_rs::{
    DepthLayer, Placement, WindowInfo, WindowManagementStrategy,
    bindings::{
        miracle_placement_t, miracle_plugin_animation_frame_data_t,
        miracle_plugin_animation_frame_result_t, miracle_point_t, miracle_window_info_t,
    },
    get_output_at,
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
    window_info: *const miracle_window_info_t,
) {
    // TODO: Pass down the name of the window properly.
    let window_info =
        unsafe { WindowInfo::from_c_with_name(window_info.as_ref().unwrap(), String::from("")) };
    let application = window_info.application();
    let mut placement: Placement = Default::default();

    // Example: Always place gedit on workspace 3.
    if let Some(application) = application {
        if application.name == "gedit" {
            placement.strategy = WindowManagementStrategy::Tiled;
            
        }
    } else {
        placement.strategy = WindowManagementStrategy::System;
    }

    unsafe {
        placement.set_c(&mut *result);
    }
}
