use miracle_plugin_rs::{
    bindings::{
        miracle_placement_t, miracle_plugin_animation_frame_data_t,
        miracle_plugin_animation_frame_result_t, miracle_point_t, miracle_window_info_t,
    },
    Context, DepthLayer, Output, Placement, WindowInfo, WindowManagementStrategy,
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
    let mut context = Context {};
    let window_info = unsafe { WindowInfo::from_c(window_info.as_ref().unwrap()) };
    let application = context.get_application(&window_info);
    let mut placement: Placement = Default::default();
    let num_outputs = context.num_outputs();
    if let Some(application) = application {
        let output = context.get_output_at(0);
        let workspace = context.get_window_workspace(&window_info);
        if application.name == "gedit" {
            placement.strategy = WindowManagementStrategy::Freestyle;
            if let Some(output) = output {
                placement.freestyle.top_left.x = output.size.width / 2;
                placement.freestyle.top_left.y = output.size.height / 2;
            } else {
                placement.freestyle.top_left.x = 100;
                placement.freestyle.top_left.y = 100;
            }

            if let Some(workspace) = workspace {
                if let Some(number) = workspace.number {
                    if number == 2 {
                        placement.freestyle.top_left.x = 0;
                        placement.freestyle.top_left.y = 0;
                    }
                }
            }

            placement.freestyle.size.width = 800;
            placement.freestyle.size.height = 600;
            placement.freestyle.depth_layer = DepthLayer::Application;
        } else {
            placement.strategy = WindowManagementStrategy::System;
        }
    } else {
        placement.strategy = WindowManagementStrategy::System;
    }

    unsafe {
        placement.set_c(&mut *result);
    }
}
