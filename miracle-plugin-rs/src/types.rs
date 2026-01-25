//! Rust-friendly wrappers for the C bindings.
//!
//! This module provides idiomatic Rust types that can be converted
//! to and from their C equivalents in the `bindings` module.

use crate::bindings;

/// Plugin version constant.
pub const PLUGIN_VERSION: u32 = bindings::MIRACLE_PLUGIN_VERSION;

// ============================================================================
// Mir Enums
// ============================================================================

/// Attributes of a window that the client and server/shell may wish to
/// get or set over the wire.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u32)]
pub enum WindowAttrib {
    Type = 0,
    State = 1,
    Focus = 3,
    Dpi = 4,
    Visibility = 5,
    PreferredOrientation = 6,
}

impl From<WindowAttrib> for bindings::MirWindowAttrib {
    fn from(value: WindowAttrib) -> Self {
        value as bindings::MirWindowAttrib
    }
}

impl TryFrom<bindings::MirWindowAttrib> for WindowAttrib {
    type Error = ();

    fn try_from(value: bindings::MirWindowAttrib) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Type),
            1 => Ok(Self::State),
            3 => Ok(Self::Focus),
            4 => Ok(Self::Dpi),
            5 => Ok(Self::Visibility),
            6 => Ok(Self::PreferredOrientation),
            _ => Err(()),
        }
    }
}

/// Window type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum WindowType {
    /// AKA "regular"
    #[default]
    Normal = 0,
    /// AKA "floating"
    Utility = 1,
    Dialog = 2,
    Gloss = 3,
    Freestyle = 4,
    Menu = 5,
    /// AKA "OSK" or handwriting etc.
    InputMethod = 6,
    /// AKA "toolbox"/"toolbar"
    Satellite = 7,
    /// AKA "tooltip"
    Tip = 8,
    Decoration = 9,
}

impl From<WindowType> for bindings::MirWindowType {
    fn from(value: WindowType) -> Self {
        value as bindings::MirWindowType
    }
}

impl TryFrom<bindings::MirWindowType> for WindowType {
    type Error = ();

    fn try_from(value: bindings::MirWindowType) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Normal),
            1 => Ok(Self::Utility),
            2 => Ok(Self::Dialog),
            3 => Ok(Self::Gloss),
            4 => Ok(Self::Freestyle),
            5 => Ok(Self::Menu),
            6 => Ok(Self::InputMethod),
            7 => Ok(Self::Satellite),
            8 => Ok(Self::Tip),
            9 => Ok(Self::Decoration),
            _ => Err(()),
        }
    }
}

/// Window state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum WindowState {
    #[default]
    Unknown = 0,
    Restored = 1,
    Minimized = 2,
    Maximized = 3,
    VertMaximized = 4,
    Fullscreen = 5,
    HorizMaximized = 6,
    Hidden = 7,
    /// Used for panels, notifications and other windows attached to output edges.
    Attached = 8,
}

impl From<WindowState> for bindings::MirWindowState {
    fn from(value: WindowState) -> Self {
        value as bindings::MirWindowState
    }
}

impl TryFrom<bindings::MirWindowState> for WindowState {
    type Error = ();

    fn try_from(value: bindings::MirWindowState) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::Restored),
            2 => Ok(Self::Minimized),
            3 => Ok(Self::Maximized),
            4 => Ok(Self::VertMaximized),
            5 => Ok(Self::Fullscreen),
            6 => Ok(Self::HorizMaximized),
            7 => Ok(Self::Hidden),
            8 => Ok(Self::Attached),
            _ => Err(()),
        }
    }
}

/// Window focus state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum WindowFocusState {
    /// Inactive and does not have focus.
    #[default]
    Unfocused = 0,
    /// Active and has keyboard focus.
    Focused = 1,
    /// Active but does not have keyboard focus.
    Active = 2,
}

impl From<WindowFocusState> for bindings::MirWindowFocusState {
    fn from(value: WindowFocusState) -> Self {
        value as bindings::MirWindowFocusState
    }
}

impl TryFrom<bindings::MirWindowFocusState> for WindowFocusState {
    type Error = ();

    fn try_from(value: bindings::MirWindowFocusState) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unfocused),
            1 => Ok(Self::Focused),
            2 => Ok(Self::Active),
            _ => Err(()),
        }
    }
}

/// Window visibility state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum WindowVisibility {
    #[default]
    Occluded = 0,
    Exposed = 1,
}

impl From<WindowVisibility> for bindings::MirWindowVisibility {
    fn from(value: WindowVisibility) -> Self {
        value as bindings::MirWindowVisibility
    }
}

impl TryFrom<bindings::MirWindowVisibility> for WindowVisibility {
    type Error = ();

    fn try_from(value: bindings::MirWindowVisibility) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Occluded),
            1 => Ok(Self::Exposed),
            _ => Err(()),
        }
    }
}

/// Lifecycle state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u32)]
pub enum LifecycleState {
    WillSuspend = 0,
    Resumed = 1,
    ConnectionLost = 2,
}

impl From<LifecycleState> for bindings::MirLifecycleState {
    fn from(value: LifecycleState) -> Self {
        value as bindings::MirLifecycleState
    }
}

impl TryFrom<bindings::MirLifecycleState> for LifecycleState {
    type Error = ();

    fn try_from(value: bindings::MirLifecycleState) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::WillSuspend),
            1 => Ok(Self::Resumed),
            2 => Ok(Self::ConnectionLost),
            _ => Err(()),
        }
    }
}

/// Power mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PowerMode {
    #[default]
    On = 0,
    Standby = 1,
    Suspend = 2,
    Off = 3,
}

impl From<PowerMode> for bindings::MirPowerMode {
    fn from(value: PowerMode) -> Self {
        value as bindings::MirPowerMode
    }
}

impl TryFrom<bindings::MirPowerMode> for PowerMode {
    type Error = ();

    fn try_from(value: bindings::MirPowerMode) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::On),
            1 => Ok(Self::Standby),
            2 => Ok(Self::Suspend),
            3 => Ok(Self::Off),
            _ => Err(()),
        }
    }
}

/// Output type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum OutputType {
    #[default]
    Unknown = 0,
    Vga = 1,
    DviI = 2,
    DviD = 3,
    DviA = 4,
    Composite = 5,
    SVideo = 6,
    Lvds = 7,
    Component = 8,
    NinePinDin = 9,
    DisplayPort = 10,
    HdmiA = 11,
    HdmiB = 12,
    Tv = 13,
    Edp = 14,
    Virtual = 15,
    Dsi = 16,
    Dpi = 17,
}

impl From<OutputType> for bindings::MirOutputType {
    fn from(value: OutputType) -> Self {
        value as bindings::MirOutputType
    }
}

impl TryFrom<bindings::MirOutputType> for OutputType {
    type Error = ();

    fn try_from(value: bindings::MirOutputType) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::Vga),
            2 => Ok(Self::DviI),
            3 => Ok(Self::DviD),
            4 => Ok(Self::DviA),
            5 => Ok(Self::Composite),
            6 => Ok(Self::SVideo),
            7 => Ok(Self::Lvds),
            8 => Ok(Self::Component),
            9 => Ok(Self::NinePinDin),
            10 => Ok(Self::DisplayPort),
            11 => Ok(Self::HdmiA),
            12 => Ok(Self::HdmiB),
            13 => Ok(Self::Tv),
            14 => Ok(Self::Edp),
            15 => Ok(Self::Virtual),
            16 => Ok(Self::Dsi),
            17 => Ok(Self::Dpi),
            _ => Err(()),
        }
    }
}

/// Pixel format.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PixelFormat {
    #[default]
    Invalid = 0,
    Abgr8888 = 1,
    Xbgr8888 = 2,
    Argb8888 = 3,
    Xrgb8888 = 4,
    Bgr888 = 5,
    Rgb888 = 6,
    Rgb565 = 7,
    Rgba5551 = 8,
    Rgba4444 = 9,
}

impl From<PixelFormat> for bindings::MirPixelFormat {
    fn from(value: PixelFormat) -> Self {
        value as bindings::MirPixelFormat
    }
}

impl TryFrom<bindings::MirPixelFormat> for PixelFormat {
    type Error = ();

    fn try_from(value: bindings::MirPixelFormat) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Invalid),
            1 => Ok(Self::Abgr8888),
            2 => Ok(Self::Xbgr8888),
            3 => Ok(Self::Argb8888),
            4 => Ok(Self::Xrgb8888),
            5 => Ok(Self::Bgr888),
            6 => Ok(Self::Rgb888),
            7 => Ok(Self::Rgb565),
            8 => Ok(Self::Rgba5551),
            9 => Ok(Self::Rgba4444),
            _ => Err(()),
        }
    }
}

/// Orientation (rotations are counter-clockwise).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum Orientation {
    #[default]
    Normal = 0,
    Left = 90,
    Inverted = 180,
    Right = 270,
}

impl From<Orientation> for bindings::MirOrientation {
    fn from(value: Orientation) -> Self {
        value as bindings::MirOrientation
    }
}

impl TryFrom<bindings::MirOrientation> for Orientation {
    type Error = ();

    fn try_from(value: bindings::MirOrientation) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Normal),
            90 => Ok(Self::Left),
            180 => Ok(Self::Inverted),
            270 => Ok(Self::Right),
            _ => Err(()),
        }
    }
}

/// Mirror mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum MirrorMode {
    #[default]
    None = 0,
    Vertical = 1,
    Horizontal = 2,
}

impl From<MirrorMode> for bindings::MirMirrorMode {
    fn from(value: MirrorMode) -> Self {
        value as bindings::MirMirrorMode
    }
}

impl TryFrom<bindings::MirMirrorMode> for MirrorMode {
    type Error = ();

    fn try_from(value: bindings::MirMirrorMode) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Vertical),
            2 => Ok(Self::Horizontal),
            _ => Err(()),
        }
    }
}

/// Placement gravity reference point.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PlacementGravity {
    #[default]
    Center = 0,
    West = 1,
    East = 2,
    North = 4,
    South = 8,
    Northwest = 5,
    Northeast = 6,
    Southwest = 9,
    Southeast = 10,
}

impl From<PlacementGravity> for bindings::MirPlacementGravity {
    fn from(value: PlacementGravity) -> Self {
        value as bindings::MirPlacementGravity
    }
}

impl TryFrom<bindings::MirPlacementGravity> for PlacementGravity {
    type Error = ();

    fn try_from(value: bindings::MirPlacementGravity) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Center),
            1 => Ok(Self::West),
            2 => Ok(Self::East),
            4 => Ok(Self::North),
            8 => Ok(Self::South),
            5 => Ok(Self::Northwest),
            6 => Ok(Self::Northeast),
            9 => Ok(Self::Southwest),
            10 => Ok(Self::Southeast),
            _ => Err(()),
        }
    }
}

bitflags::bitflags! {
    /// Positioning hints for aligning a window relative to a rectangle.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
    pub struct PlacementHints: u32 {
        /// Allow flipping anchors horizontally.
        const FLIP_X = 1;
        /// Allow flipping anchors vertically.
        const FLIP_Y = 2;
        /// Allow sliding window horizontally.
        const SLIDE_X = 4;
        /// Allow sliding window vertically.
        const SLIDE_Y = 8;
        /// Allow resizing window horizontally.
        const RESIZE_X = 16;
        /// Allow resizing window vertically.
        const RESIZE_Y = 32;
        /// Allow flipping aux_anchor to opposite corner.
        const ANTIPODES = 64;
        /// Allow flipping anchors on both axes.
        const FLIP_ANY = 3;
        /// Allow sliding window on both axes.
        const SLIDE_ANY = 12;
        /// Allow resizing window on both axes.
        const RESIZE_ANY = 48;
    }
}

impl From<PlacementHints> for bindings::MirPlacementHints {
    fn from(value: PlacementHints) -> Self {
        value.bits()
    }
}

impl From<bindings::MirPlacementHints> for PlacementHints {
    fn from(value: bindings::MirPlacementHints) -> Self {
        Self::from_bits_truncate(value)
    }
}

/// Resize edge hints.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum ResizeEdge {
    #[default]
    None = 0,
    West = 1,
    East = 2,
    North = 4,
    South = 8,
    Northwest = 5,
    Northeast = 6,
    Southwest = 9,
    Southeast = 10,
}

impl From<ResizeEdge> for bindings::MirResizeEdge {
    fn from(value: ResizeEdge) -> Self {
        value as bindings::MirResizeEdge
    }
}

impl TryFrom<bindings::MirResizeEdge> for ResizeEdge {
    type Error = ();

    fn try_from(value: bindings::MirResizeEdge) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::West),
            2 => Ok(Self::East),
            4 => Ok(Self::North),
            8 => Ok(Self::South),
            5 => Ok(Self::Northwest),
            6 => Ok(Self::Northeast),
            9 => Ok(Self::Southwest),
            10 => Ok(Self::Southeast),
            _ => Err(()),
        }
    }
}

/// Form factor associated with a physical output.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum FormFactor {
    #[default]
    Unknown = 0,
    Phone = 1,
    Tablet = 2,
    Monitor = 3,
    Tv = 4,
    Projector = 5,
}

impl From<FormFactor> for bindings::MirFormFactor {
    fn from(value: FormFactor) -> Self {
        value as bindings::MirFormFactor
    }
}

impl TryFrom<bindings::MirFormFactor> for FormFactor {
    type Error = ();

    fn try_from(value: bindings::MirFormFactor) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::Phone),
            2 => Ok(Self::Tablet),
            3 => Ok(Self::Monitor),
            4 => Ok(Self::Tv),
            5 => Ok(Self::Projector),
            _ => Err(()),
        }
    }
}

/// Subpixel arrangement.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum SubpixelArrangement {
    #[default]
    Unknown = 0,
    HorizontalRgb = 1,
    HorizontalBgr = 2,
    VerticalRgb = 3,
    VerticalBgr = 4,
    None = 5,
}

impl From<SubpixelArrangement> for bindings::MirSubpixelArrangement {
    fn from(value: SubpixelArrangement) -> Self {
        value as bindings::MirSubpixelArrangement
    }
}

impl TryFrom<bindings::MirSubpixelArrangement> for SubpixelArrangement {
    type Error = ();

    fn try_from(value: bindings::MirSubpixelArrangement) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::HorizontalRgb),
            2 => Ok(Self::HorizontalBgr),
            3 => Ok(Self::VerticalRgb),
            4 => Ok(Self::VerticalBgr),
            5 => Ok(Self::None),
            _ => Err(()),
        }
    }
}

/// Shell chrome level.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum ShellChrome {
    #[default]
    Normal = 0,
    Low = 1,
}

impl From<ShellChrome> for bindings::MirShellChrome {
    fn from(value: ShellChrome) -> Self {
        value as bindings::MirShellChrome
    }
}

impl TryFrom<bindings::MirShellChrome> for ShellChrome {
    type Error = ();

    fn try_from(value: bindings::MirShellChrome) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Normal),
            1 => Ok(Self::Low),
            _ => Err(()),
        }
    }
}

/// Pointer confinement state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PointerConfinementState {
    #[default]
    Unconfined = 0,
    ConfinedOneshot = 1,
    ConfinedPersistent = 2,
    LockedOneshot = 3,
    LockedPersistent = 4,
}

impl From<PointerConfinementState> for bindings::MirPointerConfinementState {
    fn from(value: PointerConfinementState) -> Self {
        value as bindings::MirPointerConfinementState
    }
}

impl TryFrom<bindings::MirPointerConfinementState> for PointerConfinementState {
    type Error = ();

    fn try_from(value: bindings::MirPointerConfinementState) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unconfined),
            1 => Ok(Self::ConfinedOneshot),
            2 => Ok(Self::ConfinedPersistent),
            3 => Ok(Self::LockedOneshot),
            4 => Ok(Self::LockedPersistent),
            _ => Err(()),
        }
    }
}

/// Depth layer controls Z ordering of surfaces.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum DepthLayer {
    /// For desktop backgrounds (lowest layer).
    Background = 0,
    /// For panels or other controls/decorations below normal windows.
    Below = 1,
    /// For normal application windows.
    #[default]
    Application = 2,
    /// For always-on-top application windows.
    AlwaysOnTop = 3,
    /// For panels or notifications that want to be above normal windows.
    Above = 4,
    /// For overlays such as lock screens (highest layer).
    Overlay = 5,
}

impl From<DepthLayer> for bindings::MirDepthLayer {
    fn from(value: DepthLayer) -> Self {
        value as bindings::MirDepthLayer
    }
}

impl TryFrom<bindings::MirDepthLayer> for DepthLayer {
    type Error = ();

    fn try_from(value: bindings::MirDepthLayer) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Background),
            1 => Ok(Self::Below),
            2 => Ok(Self::Application),
            3 => Ok(Self::AlwaysOnTop),
            4 => Ok(Self::Above),
            5 => Ok(Self::Overlay),
            _ => Err(()),
        }
    }
}

/// Focus mode controls how a surface gains and loses focus.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum FocusMode {
    /// The surface can gain and lose focus normally.
    #[default]
    Focusable = 0,
    /// The surface will never be given focus.
    Disabled = 1,
    /// Takes focus if possible and prevents focus from leaving.
    Grabbing = 2,
}

impl From<FocusMode> for bindings::MirFocusMode {
    fn from(value: FocusMode) -> Self {
        value as bindings::MirFocusMode
    }
}

impl TryFrom<bindings::MirFocusMode> for FocusMode {
    type Error = ();

    fn try_from(value: bindings::MirFocusMode) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Focusable),
            1 => Ok(Self::Disabled),
            2 => Ok(Self::Grabbing),
            _ => Err(()),
        }
    }
}

bitflags::bitflags! {
    /// Hints describing which edges of a surface are considered adjacent
    /// to another part of the tiling grid.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
    pub struct TiledEdge: u32 {
        const NONE = 0;
        const NORTH = 1;
        const EAST = 2;
        const SOUTH = 4;
        const WEST = 8;
    }
}

impl From<TiledEdge> for bindings::MirTiledEdge {
    fn from(value: TiledEdge) -> Self {
        value.bits()
    }
}

impl From<bindings::MirTiledEdge> for TiledEdge {
    fn from(value: bindings::MirTiledEdge) -> Self {
        Self::from_bits_truncate(value)
    }
}

/// Filters that can be applied to output.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum OutputFilter {
    #[default]
    None = 0,
    Grayscale = 1,
    Invert = 2,
}

impl From<OutputFilter> for bindings::MirOutputFilter {
    fn from(value: OutputFilter) -> Self {
        value as bindings::MirOutputFilter
    }
}

impl TryFrom<bindings::MirOutputFilter> for OutputFilter {
    type Error = ();

    fn try_from(value: bindings::MirOutputFilter) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Grayscale),
            2 => Ok(Self::Invert),
            _ => Err(()),
        }
    }
}

// ============================================================================
// Miracle Enums
// ============================================================================

/// Container type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum ContainerType {
    /// The container has a single window in it.
    #[default]
    Window = 0,
    /// The container has multiple children in it.
    Parent = 1,
}

impl From<ContainerType> for bindings::miracle_container_type {
    fn from(value: ContainerType) -> Self {
        value as bindings::miracle_container_type
    }
}

impl TryFrom<bindings::miracle_container_type> for ContainerType {
    type Error = ();

    fn try_from(value: bindings::miracle_container_type) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Window),
            1 => Ok(Self::Parent),
            _ => Err(()),
        }
    }
}

/// Layout scheme for a container.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum LayoutScheme {
    #[default]
    None = 0,
    Horizontal = 1,
    Vertical = 2,
    Tabbed = 3,
    Stacking = 4,
}

impl From<LayoutScheme> for bindings::miracle_layout_scheme {
    fn from(value: LayoutScheme) -> Self {
        value as bindings::miracle_layout_scheme
    }
}

impl TryFrom<bindings::miracle_layout_scheme> for LayoutScheme {
    type Error = ();

    fn try_from(value: bindings::miracle_layout_scheme) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Horizontal),
            2 => Ok(Self::Vertical),
            3 => Ok(Self::Tabbed),
            4 => Ok(Self::Stacking),
            _ => Err(()),
        }
    }
}

/// Window management strategy.
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

// ============================================================================
// Structs
// ============================================================================

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

impl From<Point> for bindings::miracle_point_t {
    fn from(value: Point) -> Self {
        Self {
            x: value.x,
            y: value.y,
        }
    }
}

impl From<bindings::miracle_point_t> for Point {
    fn from(value: bindings::miracle_point_t) -> Self {
        Self {
            x: value.x,
            y: value.y,
        }
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

impl From<Size> for bindings::miracle_size_t {
    fn from(value: Size) -> Self {
        Self {
            w: value.width,
            h: value.height,
        }
    }
}

impl From<bindings::miracle_size_t> for Size {
    fn from(value: bindings::miracle_size_t) -> Self {
        Self {
            width: value.w,
            height: value.h,
        }
    }
}

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

/// Animation frame data provided to the plugin.
#[derive(Debug, Clone, Copy, Default)]
pub struct AnimationFrameData {
    /// The runtime of the animation frame in seconds.
    pub runtime_seconds: f32,
    /// The total duration of the animation in seconds.
    pub duration_seconds: f32,
    /// The origin area.
    pub origin: Rect,
    /// The destination area.
    pub destination: Rect,
    /// The opacity at the start of the animation.
    pub opacity_start: f32,
    /// The opacity at the end of the animation.
    pub opacity_end: f32,
}

impl From<bindings::miracle_plugin_animation_frame_data_t> for AnimationFrameData {
    fn from(value: bindings::miracle_plugin_animation_frame_data_t) -> Self {
        Self {
            runtime_seconds: value.runtime_seconds,
            duration_seconds: value.duration_seconds,
            origin: Rect::from_array(value.origin),
            destination: Rect::from_array(value.destination),
            opacity_start: value.opacity_start,
            opacity_end: value.opacity_end,
        }
    }
}

impl From<AnimationFrameData> for bindings::miracle_plugin_animation_frame_data_t {
    fn from(value: AnimationFrameData) -> Self {
        Self {
            runtime_seconds: value.runtime_seconds,
            duration_seconds: value.duration_seconds,
            origin: value.origin.to_array(),
            destination: value.destination.to_array(),
            opacity_start: value.opacity_start,
            opacity_end: value.opacity_end,
        }
    }
}

/// Result of an animation frame computation.
#[derive(Debug, Clone, Copy, Default)]
pub struct AnimationFrameResult {
    /// If true, the animation is considered completed.
    pub completed: bool,
    /// The area rectangle (if set).
    pub area: Option<Rect>,
    /// The 4x4 transform matrix (column-major, if set).
    pub transform: Option<[f32; 16]>,
    /// The opacity value (if set).
    pub opacity: Option<f32>,
}

impl From<AnimationFrameResult> for bindings::miracle_plugin_animation_frame_result_t {
    fn from(value: AnimationFrameResult) -> Self {
        Self {
            completed: value.completed as i32,
            has_area: value.area.is_some() as i32,
            area: value.area.map(|r| r.to_array()).unwrap_or_default(),
            has_transform: value.transform.is_some() as i32,
            transform: value.transform.unwrap_or_default(),
            has_opacity: value.opacity.is_some() as i32,
            opacity: value.opacity.unwrap_or_default(),
        }
    }
}

impl From<bindings::miracle_plugin_animation_frame_result_t> for AnimationFrameResult {
    fn from(value: bindings::miracle_plugin_animation_frame_result_t) -> Self {
        Self {
            completed: value.completed != 0,
            area: if value.has_area != 0 {
                Some(Rect::from_array(value.area))
            } else {
                None
            },
            transform: if value.has_transform != 0 {
                Some(value.transform)
            } else {
                None
            },
            opacity: if value.has_opacity != 0 {
                Some(value.opacity)
            } else {
                None
            },
        }
    }
}

/// Application information.
#[derive(Debug, Clone)]
pub struct ApplicationInfo {
    /// The name of the application.
    pub name: String,
}

impl ApplicationInfo {
    /// Create from the C struct.
    ///
    /// # Safety
    /// The `application_name` pointer must be valid and null-terminated.
    pub unsafe fn from_c(value: &bindings::miracle_application_info_t) -> Self {
        let name = if value.application_name.is_null() {
            String::new()
        } else {
            unsafe {
                std::ffi::CStr::from_ptr(value.application_name)
                    .to_string_lossy()
                    .into_owned()
            }
        };
        Self { name }
    }
}

/// Window information.
#[derive(Debug, Clone)]
pub struct WindowInfo {
    /// The type of this window.
    pub window_type: WindowType,
    /// The state of the window.
    pub state: WindowState,
    /// The position of the window.
    pub top_left: Point,
    /// The size of the window.
    pub size: Size,
    /// The title of the window.
    pub title: String,
    /// The depth layer of the window.
    pub depth_layer: DepthLayer,
    /// Internal pointer for C interop.
    internal: u64,
}

impl WindowInfo {
    /// Create from the C struct.
    ///
    /// # Safety
    /// The `title` pointer must be valid and null-terminated.
    pub unsafe fn from_c(value: &bindings::miracle_window_info_t) -> Self {
        let title = if value.title.is_null() {
            String::new()
        } else {
            unsafe {
                std::ffi::CStr::from_ptr(value.title)
                    .to_string_lossy()
                    .into_owned()
            }
        };
        Self {
            window_type: WindowType::try_from(value.window_type).unwrap_or_default(),
            state: WindowState::try_from(value.state).unwrap_or_default(),
            top_left: value.top_left.into(),
            size: value.size.into(),
            title,
            depth_layer: DepthLayer::try_from(value.depth_layer).unwrap_or_default(),
            internal: value.internal,
        }
    }

    /// Get the raw C struct for interop.
    ///
    /// Note: The title pointer will be null in the returned struct.
    /// Use this only for passing to FFI functions that use the internal pointer.
    pub fn as_c(&self) -> bindings::miracle_window_info_t {
        bindings::miracle_window_info_t {
            window_type: self.window_type.into(),
            state: self.state.into(),
            top_left: self.top_left.into(),
            size: self.size.into(),
            title: std::ptr::null(),
            depth_layer: self.depth_layer.into(),
            internal: self.internal,
        }
    }
}

/// Container in a tree.
#[derive(Debug, Clone, Copy)]
pub struct Container {
    /// The type of the container.
    pub container_type: ContainerType,
    /// Whether the container is floating within its workspace.
    pub is_floating: bool,
    /// How the container is laying out its content.
    pub layout_scheme: LayoutScheme,
    /// The number of child containers.
    pub num_children: u32,
    /// Internal pointer for C interop.
    internal: u64,
}

impl Container {
    /// Get the raw C struct for interop.
    pub fn as_c(&self) -> bindings::miracle_container_t {
        bindings::miracle_container_t {
            type_: self.container_type.into(),
            is_floating: self.is_floating as i32,
            layout_scheme: self.layout_scheme.into(),
            num_child_containers: self.num_children,
            internal: self.internal,
        }
    }
}

impl From<bindings::miracle_container_t> for Container {
    fn from(value: bindings::miracle_container_t) -> Self {
        Self {
            container_type: ContainerType::try_from(value.type_).unwrap_or_default(),
            is_floating: value.is_floating != 0,
            layout_scheme: LayoutScheme::try_from(value.layout_scheme).unwrap_or_default(),
            num_children: value.num_child_containers,
            internal: value.internal,
        }
    }
}

impl From<Container> for bindings::miracle_container_t {
    fn from(value: Container) -> Self {
        value.as_c()
    }
}

/// Workspace information.
#[derive(Debug, Clone)]
pub struct Workspace {
    /// Whether the workspace is set.
    pub is_set: bool,
    /// The workspace number (if set).
    pub number: Option<u32>,
    /// The workspace name (if set).
    pub name: Option<String>,
    /// The number of container trees in this workspace.
    pub num_trees: u32,
    /// Internal pointer for C interop.
    internal: u64,
}

impl Workspace {
    /// Create from the C struct.
    ///
    /// # Safety
    /// The `name` pointer must be valid and null-terminated if `has_name` is true.
    pub unsafe fn from_c(value: &bindings::miracle_workspace_t) -> Self {
        let name = if value.has_name != 0 && !value.name.is_null() {
            Some(unsafe {
                std::ffi::CStr::from_ptr(value.name)
                    .to_string_lossy()
                    .into_owned()
            })
        } else {
            None
        };

        Self {
            is_set: value.is_set != 0,
            number: if value.has_number != 0 {
                Some(value.number)
            } else {
                None
            },
            name,
            num_trees: value.num_trees,
            internal: value.internal,
        }
    }

    /// Get the internal pointer for C interop.
    pub fn internal_ptr(&self) -> u64 {
        self.internal
    }

    /// Get the raw C struct for interop.
    ///
    /// Note: The name pointer will be null in the returned struct.
    /// Use this only for passing to FFI functions that use the internal pointer.
    pub fn as_c(&self) -> bindings::miracle_workspace_t {
        bindings::miracle_workspace_t {
            is_set: self.is_set as i32,
            has_number: self.number.is_some() as i32,
            number: self.number.unwrap_or(0),
            has_name: self.name.is_some() as i32,
            name: std::ptr::null(),
            num_trees: self.num_trees,
            internal: self.internal,
        }
    }
}

/// Output information.
#[derive(Debug, Clone)]
pub struct Output {
    /// Whether the output is set.
    pub is_set: bool,
    /// The position of the output.
    pub position: Point,
    /// The size of the output.
    pub size: Size,
    /// The name of the output.
    pub name: String,
    /// Whether this is the primary output.
    pub is_primary: bool,
    /// Internal pointer for C interop.
    internal: u64,
}

impl Output {
    /// Create from the C struct.
    ///
    /// # Safety
    /// The `name` pointer must be valid and null-terminated.
    pub unsafe fn from_c(value: &bindings::miracle_output_t) -> Self {
        let name = if value.name.is_null() {
            String::new()
        } else {
            unsafe {
                std::ffi::CStr::from_ptr(value.name)
                    .to_string_lossy()
                    .into_owned()
            }
        };

        Self {
            is_set: value.is_set != 0,
            position: value.position.into(),
            size: value.size.into(),
            name,
            is_primary: value.is_primary != 0,
            internal: value.internal,
        }
    }

    /// Get the internal pointer for C interop.
    pub fn internal_ptr(&self) -> u64 {
        self.internal
    }
}

/// Tiled placement configuration.
#[derive(Debug, Clone, Copy, Default)]
pub struct TiledPlacement {
    /// The parent container.
    pub parent: bindings::miracle_container_t,
    /// The index at which to place the container.
    pub index: u32,
    /// The requested layout scheme.
    pub layout_scheme: LayoutScheme,
}

impl From<TiledPlacement> for bindings::miracle_tiled_placement_t {
    fn from(value: TiledPlacement) -> Self {
        Self {
            parent: value.parent,
            index: value.index,
            layout_scheme: value.layout_scheme.into(),
        }
    }
}

impl From<bindings::miracle_tiled_placement_t> for TiledPlacement {
    fn from(value: bindings::miracle_tiled_placement_t) -> Self {
        Self {
            parent: value.parent,
            index: value.index,
            layout_scheme: LayoutScheme::try_from(value.layout_scheme).unwrap_or_default(),
        }
    }
}

/// Freestyle placement configuration.
#[derive(Debug, Clone, Copy, Default)]
pub struct FreestylePlacement {
    /// The top left position.
    pub top_left: Point,
    /// The depth layer.
    pub depth_layer: DepthLayer,
    /// The workspace (0 for always visible).
    /// This is a 32-bit WASM linear memory pointer.
    pub workspace: u32,
    /// The size.
    pub size: Size,
}

impl From<FreestylePlacement> for bindings::miracle_freestyle_placement_t {
    fn from(value: FreestylePlacement) -> Self {
        Self {
            top_left: value.top_left.into(),
            depth_layer: value.depth_layer.into(),
            workspace: value.workspace,
            size: value.size.into(),
        }
    }
}

impl From<bindings::miracle_freestyle_placement_t> for FreestylePlacement {
    fn from(value: bindings::miracle_freestyle_placement_t) -> Self {
        Self {
            top_left: value.top_left.into(),
            depth_layer: DepthLayer::try_from(value.depth_layer).unwrap_or_default(),
            workspace: value.workspace,
            size: value.size.into(),
        }
    }
}

/// Placement configuration.
#[derive(Debug, Clone, Copy)]
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
        out.freestyle_placement = self.freestyle.into();
        out.titled_placement = self.tiled.into();
    }
}

impl From<Placement> for bindings::miracle_placement_t {
    fn from(value: Placement) -> Self {
        Self {
            strategy: value.strategy.into(),
            freestyle_placement: value.freestyle.into(),
            titled_placement: value.tiled.into(),
        }
    }
}

impl From<bindings::miracle_placement_t> for Placement {
    fn from(value: bindings::miracle_placement_t) -> Self {
        Self {
            strategy: WindowManagementStrategy::try_from(value.strategy).unwrap_or_default(),
            freestyle: value.freestyle_placement.into(),
            tiled: value.titled_placement.into(),
        }
    }
}

// ============================================================================
// Host Imports
// ============================================================================

/// Raw FFI declarations for host-provided functions.
///
/// These functions are imported from the WASM host (miracle-wm) at runtime.
mod ffi {
    use crate::bindings;

    unsafe extern "C" {
        /// Retrieve the application info for a given window.
        pub fn miracle_window_info_get_application(
            context: *mut bindings::miracle_context_t,
            window_info: *mut bindings::miracle_window_info_t,
        ) -> bindings::miracle_application_info_t;

        /// Retrieve the workspace that a window is on.
        pub fn miracle_window_info_get_workspace(
            context: *mut bindings::miracle_context_t,
            window_info: *mut bindings::miracle_window_info_t,
        ) -> bindings::miracle_workspace_t;

        /// Retrieve the output that a workspace is on.
        pub fn miracle_workspace_get_output(
            context: *mut bindings::miracle_context_t,
            workspace: *mut bindings::miracle_workspace_t,
        ) -> bindings::miracle_output_t;

        /// Retrieve the number of outputs.
        pub fn miracle_get_num_outputs(context: *mut bindings::miracle_context_t) -> u32;

        /// Retrieve an output by index.
        pub fn miracle_get_output_at(
            context: *mut bindings::miracle_context_t,
            index: u32,
        ) -> bindings::miracle_output_t;

        /// Retrieve a tree from a workspace by index.
        pub fn miracle_workspace_get_tree(
            context: *mut bindings::miracle_context_t,
            workspace: *mut bindings::miracle_workspace_t,
            index: u32,
        ) -> bindings::miracle_container_t;

        /// Retrieve a child container from a parent container by index.
        pub fn miracle_container_get_child_at(
            context: *mut bindings::miracle_context_t,
            container: *mut bindings::miracle_container_t,
            index: u32,
        ) -> bindings::miracle_container_t;

        /// Retrieve the window info from a window container.
        pub fn miracle_container_get_window(
            context: *mut bindings::miracle_context_t,
            container: *mut bindings::miracle_container_t,
        ) -> bindings::miracle_window_info_t;
    }
}

// ============================================================================
// Context
// ============================================================================

/// Opaque context for calling into Miracle's internals.
#[derive(Debug, Clone, Copy)]
pub struct Context {
    raw: bindings::miracle_context_t,
}

impl Context {
    /// Create a context from the raw C struct.
    pub fn from_raw(raw: bindings::miracle_context_t) -> Self {
        Self { raw }
    }

    /// Get the raw C struct.
    pub fn as_raw(&self) -> &bindings::miracle_context_t {
        &self.raw
    }

    /// Get the raw C struct mutably.
    pub fn as_raw_mut(&mut self) -> &mut bindings::miracle_context_t {
        &mut self.raw
    }

    /// Get the application info for a window.
    pub fn get_application(&mut self, window_info: &WindowInfo) -> ApplicationInfo {
        unsafe {
            let mut c_window_info = window_info.as_c();
            let result =
                ffi::miracle_window_info_get_application(self.as_raw_mut(), &mut c_window_info);
            ApplicationInfo::from_c(&result)
        }
    }

    /// Get the workspace that a window is on.
    pub fn get_window_workspace(&mut self, window_info: &WindowInfo) -> Workspace {
        unsafe {
            let mut c_window_info = window_info.as_c();
            let result =
                ffi::miracle_window_info_get_workspace(self.as_raw_mut(), &mut c_window_info);
            Workspace::from_c(&result)
        }
    }

    /// Get the output that a workspace is on.
    pub fn get_workspace_output(&mut self, workspace: &Workspace) -> Output {
        unsafe {
            let mut c_workspace = workspace.as_c();
            let result = ffi::miracle_workspace_get_output(self.as_raw_mut(), &mut c_workspace);
            Output::from_c(&result)
        }
    }

    /// Get the number of outputs.
    pub fn get_output_count(&mut self) -> u32 {
        unsafe { ffi::miracle_get_num_outputs(self.as_raw_mut()) }
    }

    /// Get an output by index.
    ///
    /// Returns `None` if the index is out of bounds.
    pub fn get_output_at(&mut self, index: u32) -> Option<Output> {
        if index >= self.get_output_count() {
            return None;
        }
        let result = unsafe { ffi::miracle_get_output_at(self.as_raw_mut(), index) };
        let output = unsafe { Output::from_c(&result) };
        if output.is_set { Some(output) } else { None }
    }

    /// Get all outputs.
    pub fn get_outputs(&mut self) -> Vec<Output> {
        let count = self.get_output_count();
        (0..count).filter_map(|i| self.get_output_at(i)).collect()
    }

    /// Get a tree from a workspace by index.
    ///
    /// Returns `None` if the index is out of bounds.
    pub fn get_workspace_tree(&mut self, workspace: &Workspace, index: u32) -> Option<Container> {
        if index >= workspace.num_trees {
            return None;
        }
        unsafe {
            let mut c_workspace = workspace.as_c();
            let result =
                ffi::miracle_workspace_get_tree(self.as_raw_mut(), &mut c_workspace, index);
            Some(Container::from(result))
        }
    }

    /// Get all trees from a workspace.
    pub fn get_workspace_trees(&mut self, workspace: &Workspace) -> Vec<Container> {
        (0..workspace.num_trees)
            .filter_map(|i| self.get_workspace_tree(workspace, i))
            .collect()
    }

    /// Get a child container from a parent container by index.
    ///
    /// Returns `None` if the index is out of bounds or if the container
    /// is not of type `Parent`.
    pub fn get_container_child_at(
        &mut self,
        container: &Container,
        index: u32,
    ) -> Option<Container> {
        if container.container_type != ContainerType::Parent || index >= container.num_children {
            return None;
        }
        unsafe {
            let mut c_container = container.as_c();
            let result =
                ffi::miracle_container_get_child_at(self.as_raw_mut(), &mut c_container, index);
            Some(Container::from(result))
        }
    }

    /// Get all children from a parent container.
    ///
    /// Returns an empty vector if the container is not of type `Parent`.
    pub fn get_container_children(&mut self, container: &Container) -> Vec<Container> {
        if container.container_type != ContainerType::Parent {
            return Vec::new();
        }
        (0..container.num_children)
            .filter_map(|i| self.get_container_child_at(container, i))
            .collect()
    }

    /// Get the window info from a window container.
    ///
    /// Returns `None` if the container is not of type `Window`.
    pub fn get_container_window(&mut self, container: &Container) -> Option<WindowInfo> {
        if container.container_type != ContainerType::Window {
            return None;
        }
        unsafe {
            let mut c_container = container.as_c();
            let result = ffi::miracle_container_get_window(self.as_raw_mut(), &mut c_container);
            Some(WindowInfo::from_c(&result))
        }
    }
}
