use super::bindings;
use super::container::*;
use super::core::*;
use super::window::*;
use super::workspace::*;

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
            parent_internal: value.parent.map_or(0, |c| c.internal),
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
    pub transform: [f32; 16],
    /// The alpha (opacity) of the window.
    ///
    /// Defaults to 1.0 (fully opaque).
    pub alpha: f32,
}

impl Default for FreestylePlacement {
    fn default() -> Self {
        Self {
            top_left: Point::default(),
            depth_layer: DepthLayer::default(),
            workspace: None,
            size: Size::default(),
            transform: [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ],
            alpha: 1.0,
        }
    }
}

impl From<FreestylePlacement> for bindings::miracle_freestyle_placement_t {
    fn from(value: FreestylePlacement) -> Self {
        Self {
            top_left: value.top_left.into(),
            depth_layer: value.depth_layer.into(),
            workspace_internal: value.workspace.map_or(0, |w| w.internal),
            size: value.size.into(),
            transform: value.transform,
            alpha: value.alpha,
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
