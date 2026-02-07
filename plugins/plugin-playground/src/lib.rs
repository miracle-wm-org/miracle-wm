use miracle_plugin_rs::{
    Container, ContainerType, LayoutScheme, Placement, TiledPlacement, WindowInfo,
    WindowManagementStrategy,
    bindings::{
        miracle_placement_t, miracle_plugin_animation_frame_data_t,
        miracle_plugin_animation_frame_result_t, miracle_point_t, miracle_window_info_t,
    },
    request_workspace,
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

fn count_windows(container: &Container) -> u32 {
    match container.container_type {
        ContainerType::Window => 1,
        ContainerType::Parent => container
            .get_children()
            .iter()
            .map(|c| count_windows(c))
            .sum(),
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
    let mut placement: Placement = Default::default();

    // Request workspace 1 by number (creates it if it doesn't exist) and focus it.
    let workspace = request_workspace(Some(1), None, true);
    if let Some(workspace) = workspace {
        let first_non_floating = workspace.trees().into_iter().find(|t| !t.is_floating);
        if let Some(tree) = first_non_floating {
            let window_count = count_windows(&tree);
            if window_count >= 2 {
                if let Some(second_child) = tree.child_at(1) {
                    let index = match second_child.container_type {
                        ContainerType::Parent => second_child.num_children,
                        ContainerType::Window => 1,
                    };
                    placement.strategy = WindowManagementStrategy::Tiled;
                    placement.tiled = TiledPlacement {
                        parent: Some(second_child),
                        index,
                        layout_scheme: LayoutScheme::Vertical,
                    };
                }
            }
        }
    } else {
        placement.strategy = WindowManagementStrategy::Freestyle;
    }

    unsafe {
        placement.set_c(&mut *result);
    }
}
