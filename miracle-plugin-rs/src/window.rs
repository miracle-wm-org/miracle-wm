use super::application::*;
use super::bindings;
use super::core::{self, *};
use super::host::*;
use super::workspace::*;
use glam::Mat4;

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
    /// The depth layer of the window.
    pub depth_layer: DepthLayer,
    /// The name of the window.
    pub name: String,
    /// The 4x4 transform matrix of the window (column-major).
    pub transform: Mat4,
    /// The alpha (opacity) of the window.
    pub alpha: f32,
    /// Internal pointer for C interop.
    internal: u64,
}

impl WindowInfo {
    /// Create from the C struct.
    ///
    /// # Safety
    /// The `title` pointer must be valid and null-terminated.
    pub unsafe fn from_c_with_name(value: &bindings::miracle_window_info_t, name: String) -> Self {
        Self {
            window_type: WindowType::try_from(value.window_type).unwrap_or_default(),
            state: WindowState::try_from(value.state).unwrap_or_default(),
            top_left: value.top_left.into(),
            size: value.size.into(),
            depth_layer: DepthLayer::try_from(value.depth_layer).unwrap_or_default(),
            name,
            transform: core::mat4_from_f32_array(value.transform),
            alpha: value.alpha,
            internal: value.internal,
        }
    }

    /// Get the application that owns this window.
    pub fn application(&self) -> Option<ApplicationInfo> {
        const NAME_BUF_LEN: usize = 256;
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let internal = miracle_window_info_get_application(
                self.internal as i64,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if internal == -1 {
                return None;
            }

            // Find the null terminator to get the actual string length
            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(ApplicationInfo {
                name,
                internal: internal as u64,
            })
        }
    }

    /// Get the workspace that this window is on.
    pub fn workspace(&self) -> Option<Workspace> {
        const NAME_BUF_LEN: usize = 256;
        let mut workspace = std::mem::MaybeUninit::<crate::bindings::miracle_workspace_t>::uninit();
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let result = miracle_window_info_get_workspace(
                self.internal as i64,
                workspace.as_mut_ptr() as i32,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if result != 0 {
                return None;
            }

            let workspace = workspace.assume_init();
            if workspace.is_set == 0 {
                return None;
            }

            // Find the null terminator to get the actual string length
            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(Workspace::from_c_with_name(&workspace, name))
        }
    }
}

impl PartialEq for WindowInfo {
    fn eq(&self, other: &Self) -> bool {
        self.internal == other.internal
    }
}

/// A handle to a window managed by this plugin, with mutation methods.
///
/// Returned by [`crate::plugin::Plugin::managed_windows`]. Wraps [`WindowInfo`] and exposes
/// all of its read-only fields via [`std::ops::Deref`], while adding setter methods that
/// call into the compositor host.
#[derive(Debug, Clone)]
pub struct PluginWindow {
    info: WindowInfo,
}

impl PluginWindow {
    pub fn from_window_info(info: WindowInfo) -> Self {
        Self { info }
    }

    /// Set the state of this window.
    pub fn set_state(&self, state: WindowState) -> Result<(), ()> {
        let r = unsafe { miracle_window_set_state(self.info.internal as i64, state as i32) };
        if r == 0 { Ok(()) } else { Err(()) }
    }

    /// Move this window to a different workspace.
    pub fn set_workspace(&self, workspace: &Workspace) -> Result<(), ()> {
        let r = unsafe {
            miracle_window_set_workspace(self.info.internal as i64, workspace.internal as i64)
        };
        if r == 0 { Ok(()) } else { Err(()) }
    }

    /// Set the position and size of this window.
    pub fn set_rectangle(&self, rect: Rectangle) -> Result<(), ()> {
        let r = unsafe {
            miracle_window_set_rectangle(
                self.info.internal as i64,
                rect.x,
                rect.y,
                rect.width,
                rect.height,
            )
        };
        if r == 0 { Ok(()) } else { Err(()) }
    }

    /// Set the 4x4 column-major transform matrix of this window.
    pub fn set_transform(&self, transform: Mat4) -> Result<(), ()> {
        let arr = transform.to_cols_array();
        let r = unsafe { miracle_window_set_transform(self.info.internal as i64, arr.as_ptr() as i32) };
        if r == 0 { Ok(()) } else { Err(()) }
    }

    /// Set the alpha (opacity) of this window.
    pub fn set_alpha(&self, alpha: f32) -> Result<(), ()> {
        let r = unsafe { miracle_window_set_alpha(self.info.internal as i64, (&alpha as *const f32) as i32) };
        if r == 0 { Ok(()) } else { Err(()) }
    }

    /// Request keyboard focus on this window.
    pub fn request_focus(&self) -> Result<(), ()> {
        let r = unsafe { miracle_window_request_focus(self.info.internal as i64) };
        if r == 0 { Ok(()) } else { Err(()) }
    }
}

impl std::ops::Deref for PluginWindow {
    type Target = WindowInfo;

    fn deref(&self) -> &Self::Target {
        &self.info
    }
}

impl PartialEq for PluginWindow {
    fn eq(&self, other: &Self) -> bool {
        self.info == other.info
    }
}
