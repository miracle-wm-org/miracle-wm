use super::bindings;
use super::container::*;
use super::core::*;
use super::window::*;
use super::workspace::*;
use glam::Mat4;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum WindowManagementStrategy {
    /// Use the system default management strategy.
    #[default]
    System = 0,
    /// Window will be placed in the tiling grid.
    Tiled = 1,
    /// Window behavior is entirely determined by the plugin.
    Freestyle = 2,
}

impl From<WindowManagementStrategy> for bindings::miracle_window_management_strategy_t {
    fn from(value: WindowManagementStrategy) -> Self {
        value as bindings::miracle_window_management_strategy_t
    }
}

impl TryFrom<bindings::miracle_window_management_strategy_t> for WindowManagementStrategy {
    type Error = ();

    fn try_from(
        value: bindings::miracle_window_management_strategy_t,
    ) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::System),
            1 => Ok(Self::Tiled),
            2 => Ok(Self::Freestyle),
            _ => Err(()),
        }
    }
}

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

/// Placement configuration.
#[derive(Debug, Clone)]
pub struct Placement {
    /// The placement strategy.
    pub strategy: WindowManagementStrategy,
    /// Freestyle placement (used if strategy is Freestyle).
    pub freestyle: FreestylePlacement,
    /// Tiled placement (used if strategy is Tiled).
    pub tiled: TiledPlacement,
}

impl Default for Placement {
    fn default() -> Self {
        Self {
            strategy: WindowManagementStrategy::default(),
            freestyle: FreestylePlacement::default(),
            tiled: TiledPlacement::default(),
        }
    }
}

impl Placement {
    /// Set the values of a C placement struct from this Placement.
    pub fn set_c(&self, out: &mut bindings::miracle_placement_t) {
        out.strategy = self.strategy.into();
        out.freestyle_placement = self.freestyle.clone().into();
        out.tiled_placement = self.tiled.clone().into();
    }
}

impl From<Placement> for bindings::miracle_placement_t {
    fn from(value: Placement) -> Self {
        Self {
            strategy: value.strategy.into(),
            freestyle_placement: value.freestyle.clone().into(),
            tiled_placement: value.tiled.clone().into(),
        }
    }
}
