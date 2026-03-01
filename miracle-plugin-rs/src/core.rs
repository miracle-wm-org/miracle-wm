use super::bindings::{miracle_point_t, miracle_size_t};
use glam::Mat4;

/// A rectangle defined by a point and size.
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Rect {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}

impl Rect {
    pub const fn new(x: f32, y: f32, width: f32, height: f32) -> Self {
        Self {
            x,
            y,
            width,
            height,
        }
    }

    pub fn from_array(arr: [f32; 4]) -> Self {
        Self {
            x: arr[0],
            y: arr[1],
            width: arr[2],
            height: arr[3],
        }
    }

    pub fn to_array(self) -> [f32; 4] {
        [self.x, self.y, self.width, self.height]
    }
}

/// A size with integer dimensions.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Size {
    pub width: i32,
    pub height: i32,
}

impl Size {
    pub const fn new(width: i32, height: i32) -> Self {
        Self { width, height }
    }
}

impl From<Size> for miracle_size_t {
    fn from(value: Size) -> Self {
        Self {
            w: value.width,
            h: value.height,
        }
    }
}

impl From<miracle_size_t> for Size {
    fn from(value: miracle_size_t) -> Self {
        Self {
            width: value.w,
            height: value.h,
        }
    }
}

/// A 2D point with integer coordinates.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

impl Point {
    pub const fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }
}

impl From<Point> for miracle_point_t {
    fn from(value: Point) -> Self {
        Self {
            x: value.x,
            y: value.y,
        }
    }
}

impl From<miracle_point_t> for Point {
    fn from(value: miracle_point_t) -> Self {
        Self {
            x: value.x,
            y: value.y,
        }
    }
}

/// A rectangle with integer position and dimensions.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Rectangle {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl Rectangle {
    pub const fn new(x: i32, y: i32, width: i32, height: i32) -> Self {
        Self { x, y, width, height }
    }
}

pub fn mat4_from_f32_array(arr: [f32; 16]) -> Mat4 {
    Mat4::from_cols_array(&arr)
}

pub fn mat4_to_f32_array(mat: Mat4) -> [f32; 16] {
    mat.to_cols_array()
}
