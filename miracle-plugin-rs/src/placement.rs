use super::bindings;
use super::container::*;
use super::core::*;
use super::window::*;
use super::workspace::*;
use glam::Mat4;

/// Placement parameters for a tiled window.
///
/// Used with [`Placement::Tiled`].
#[derive(Debug, Clone, Default)]
pub struct TiledPlacement {
    /// The parent container.
    pub parent: Option<Container>,
    /// The index at which to place the container.
    pub index: u32,
    /// The requested layout scheme.
    pub layout_scheme: LayoutScheme,
}

impl From<TiledPlacement> for bindings::miracle_tiled_placement_t {
    fn from(value: TiledPlacement) -> Self {
        Self {
            parent_internal: value.parent.map_or(0, |c| c.id()),
            index: value.index,
            layout_scheme: value.layout_scheme.into(),
        }
    }
}

/// Freestyle placement configuration.
#[derive(Debug, Clone)]
pub struct FreestylePlacement {
    /// The top left position.
    pub top_left: Point,
    /// The depth layer.
    pub depth_layer: DepthLayer,
    /// The workspace
    pub workspace: Option<Workspace>,
    /// The size.
    pub size: Size,
    /// The 4x4 transform matrix applied to the window (column-major).
    ///
    /// Defaults to the identity matrix.
    pub transform: Mat4,
    /// The alpha (opacity) of the window.
    ///
    /// Defaults to 1.0 (fully opaque).
    pub alpha: f32,
    /// Whether the window can be resized.
    ///
    /// Defaults to true.
    pub resizable: bool,
    /// Whether the window can be moved.
    ///
    /// Defaults to true.
    pub movable: bool,
}

impl Default for FreestylePlacement {
    fn default() -> Self {
        Self {
            top_left: Point::default(),
            depth_layer: DepthLayer::default(),
            workspace: None,
            size: Size::default(),
            transform: Mat4::IDENTITY,
            alpha: 1.0,
            resizable: true,
            movable: true,
        }
    }
}

impl From<FreestylePlacement> for bindings::miracle_freestyle_placement_t {
    fn from(value: FreestylePlacement) -> Self {
        Self {
            top_left: value.top_left.into(),
            depth_layer: value.depth_layer.into(),
            workspace_internal: value.workspace.map_or(0, |w| w.id()),
            size: value.size.into(),
            transform: mat4_to_f32_array(value.transform),
            alpha: value.alpha,
            resizable: value.resizable as i32,
            movable: value.movable as i32,
        }
    }
}

/// Complete placement specification returned from [`crate::plugin::Plugin::place_new_window`].
#[derive(Debug, Clone)]
pub enum Placement {
    /// Window will be placed in the tiling grid.
    Tiled(TiledPlacement),
    /// Window behavior is entirely determined by the plugin.
    Freestyle(FreestylePlacement),
}

impl From<Placement> for bindings::miracle_placement_t {
    fn from(value: Placement) -> Self {
        match value {
            Placement::Tiled(tiled) => Self {
                strategy: bindings::miracle_window_management_strategy_t_miracle_window_management_strategy_tiled,
                freestyle_placement: Default::default(),
                tiled_placement: tiled.into(),
            },
            Placement::Freestyle(freestyle) => Self {
                strategy: bindings::miracle_window_management_strategy_t_miracle_window_management_strategy_freestyle,
                freestyle_placement: freestyle.into(),
                tiled_placement: Default::default(),
            },
        }
    }
}
