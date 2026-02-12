use miracle_plugin_rs::{
    bindings::{
        miracle_placement_t, miracle_plugin_animation_frame_data_t,
        miracle_plugin_animation_frame_result_t, miracle_point_t, miracle_window_info_t,
    },
    request_workspace, Container, ContainerType, LayoutScheme, Placement, TiledPlacement,
    WindowInfo, WindowManagementStrategy, Point
};

#[unsafe(no_mangle)]
pub extern "C" fn add_points(a: miracle_point_t, b: miracle_point_t) -> miracle_point_t {
    miracle_point_t {
        x: a.x + b.x,
        y: a.y + b.y,
    }
}

fn ease_out_cubic(t: f32) -> f32 {
    let inv = 1.0 - t;
    1.0 - inv * inv * inv
}

fn ease_in_cubic(t: f32) -> f32 {
    t * t * t
}

#[unsafe(no_mangle)]
pub extern "C" fn animate(
    data: miracle_plugin_animation_frame_data_t,
) -> miracle_plugin_animation_frame_result_t {
    let progress = (data.runtime_seconds / data.duration_seconds).clamp(0.0, 1.0);

    let offset = 16.0;
    let mut area = if data.opacity_start < data.opacity_end {
        data.destination
    } else {
        data.origin
    };

    if data.opacity_start < data.opacity_end {
        // Fading in (0 -> 1): ease-out for both position and opacity
        let eased = ease_out_cubic(progress);
        let remaining = 1.0 - eased;
        let opacity = data.opacity_start + (data.opacity_end - data.opacity_start) * eased;
        area[0] -= offset * remaining;
        area[1] += offset * remaining;

        miracle_plugin_animation_frame_result_t {
            completed: 0,
            has_area: 1,
            area,
            has_transform: 0,
            transform: [0.0; 16],
            has_opacity: 1,
            opacity,
        }
    } else if data.opacity_start > data.opacity_end {
        // Fading out (1 -> 0): ease-in for both position and opacity
        let eased = ease_in_cubic(progress);
        let opacity = data.opacity_start + (data.opacity_end - data.opacity_start) * eased;
        area[0] += offset * eased;
        area[1] += offset * eased;

        miracle_plugin_animation_frame_result_t {
            completed: 0,
            has_area: 1,
            area,
            has_transform: 0,
            transform: [0.0; 16],
            has_opacity: 1,
            opacity,
        }
    } else {
        let opacity = data.opacity_end;
        miracle_plugin_animation_frame_result_t {
            completed: 0,
            has_area: 1,
            area,
            has_transform: 0,
            transform: [0.0; 16],
            has_opacity: 1,
            opacity,
        }
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
        placement.strategy = WindowManagementStrategy::Freestyle;
        placement.freestyle.top_left = Point{ x: 0, y: 0 };
        placement.freestyle.workspace = Some(workspace);
    }

    unsafe {
        placement.set_c(&mut *result);
    }
}
