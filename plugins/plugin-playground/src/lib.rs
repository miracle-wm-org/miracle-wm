#[repr(C)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

#[repr(C)]
pub struct MiracleAnimationFrameData {
    runtime_seconds: f32,
    duration_seconds: f32,
    origin: [f32; 4],
    destination: [f32; 4],
    opacity_start: f32,
    opacity_end: f32,
}

#[repr(C)]
pub struct MiracleAnimationFrameResult {
    completed: i32,
    has_area: i32,
    area: [f32; 4],
    has_transform: i32,
    transform: [f32; 16],
    has_opacity: i32,
    opacity: f32,
}

#[unsafe(no_mangle)]
pub extern "C" fn add_points(a: Point, b: Point) -> Point {
    Point {
        x: a.x + b.x,
        y: a.y + b.y,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn animate(data: MiracleAnimationFrameData) -> MiracleAnimationFrameResult {
    let progress = data.runtime_seconds / data.duration_seconds;
    let opacity = data.opacity_start + (data.opacity_end - data.opacity_start) * progress;
    MiracleAnimationFrameResult {
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

#[repr(C)]
pub struct Size {
    pub w: i32,
    pub h: i32,
}

#[repr(C)]
pub struct MiracleWindowInfo {
    pub window_type: i32,
    pub state: i32,
    pub top_left: Point,
    pub size: Size,
    pub title: *const i8,
    pub depth_layer: i32,
    pub internal: *mut core::ffi::c_void,
}

#[repr(C)]
pub struct MiraclePlacement {
    pub is_set: i32,
    pub top_left: Point,
    pub size: Size,
    pub depth_layer: i32,
}

#[repr(C)]
pub struct MiracleContext {
    pub internal: *mut core::ffi::c_void,
}

#[unsafe(no_mangle)]
pub extern "C" fn place_new_window(result: *mut MiraclePlacement, _context: *const MiracleContext, _window_info: *const MiracleWindowInfo) {
    unsafe {
        (*result).is_set = 1;
        (*result).top_left.x = 100;
        (*result).top_left.y = 100;
        (*result).size.w = 800;
        (*result).size.h = 600;
        (*result).depth_layer = 2;
    }
}
