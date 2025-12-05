#[repr(C)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

#[repr(C)]
pub struct MiracleAnimationFrameData {
    p: f32,
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
pub extern "C" fn animate(
    data: MiracleAnimationFrameData,
) -> MiracleAnimationFrameResult {
    MiracleAnimationFrameResult {
        completed: 1,
        has_area: 1,
        area: [data.origin[0], data.origin[1], data.destination[0], data.destination[1]],
        has_transform: 0,
        transform: [0.0; 16],
        has_opacity: 1,
        opacity: data.opacity_start + (data.opacity_end - data.opacity_start) * data.p,
    }
}
