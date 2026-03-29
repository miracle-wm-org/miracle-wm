use super::bindings;
use super::host::*;
use super::window::*;

/// Describes how a parent container lays out its children.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum LayoutScheme {
    /// No layout applied.
    #[default]
    None = 0,
    /// Children arranged side by side.
    Horizontal = 1,
    /// Children stacked top to bottom.
    Vertical = 2,
    /// Children shown as tabs; only the focused child is visible.
    Tabbed = 3,
    /// Children stacked visually with title bars visible.
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

/// A leaf container that holds a single window.
#[derive(Debug, Clone, Copy)]
pub struct WindowContainer {
    /// Whether the container is floating within its workspace.
    pub is_floating: bool,
    internal: u64,
}

impl WindowContainer {
    /// Get the window info from this container.
    pub fn window(&self) -> Option<WindowInfo> {
        const NAME_BUF_LEN: usize = 256;
        let mut window = std::mem::MaybeUninit::<crate::bindings::miracle_window_info_t>::uninit();
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let result = miracle_container_get_window(
                self.internal as i64,
                window.as_mut_ptr() as i32,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if result != 0 {
                return None;
            }

            let window = window.assume_init();

            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(WindowInfo::from_c_with_name(&window, name))
        }
    }
}

/// A parent container that holds multiple child containers.
#[derive(Debug, Clone, Copy)]
pub struct ParentContainer {
    /// Whether the container is floating within its workspace.
    pub is_floating: bool,
    /// How the container is laying out its children.
    pub layout_scheme: LayoutScheme,
    /// The number of child containers.
    pub num_children: u32,
    internal: u64,
}

impl ParentContainer {
    /// Get a child container by index.
    ///
    /// Returns `None` if the index is out of bounds.
    pub fn child_at(&self, index: u32) -> Option<Container> {
        if index >= self.num_children {
            return None;
        }

        let mut child_container =
            std::mem::MaybeUninit::<crate::bindings::miracle_container_t>::uninit();
        unsafe {
            let result = miracle_container_get_child_at(
                self.internal as i64,
                index,
                child_container.as_mut_ptr() as i32,
            );

            if result != 0 {
                return None;
            }

            let child_container = child_container.assume_init();
            Some(Container::from(child_container))
        }
    }

    /// Get all child containers.
    pub fn get_children(&self) -> Vec<Container> {
        (0..self.num_children)
            .filter_map(|i| self.child_at(i))
            .collect()
    }
}

/// A node in the workspace container tree.
///
/// Match on this enum to distinguish between leaf window nodes ([`Container::Window`])
/// and parent split nodes ([`Container::Parent`]). Shared operations such as [`Container::id`],
/// [`Container::parent`], and [`Container::is_floating`] are available directly on the enum.
#[derive(Debug, Clone, Copy)]
pub enum Container {
    /// A leaf node holding a single window.
    Window(WindowContainer),
    /// A split node holding child containers.
    Parent(ParentContainer),
}

impl Container {
    /// Returns the opaque internal ID used to refer to this container across API calls.
    pub fn id(&self) -> u64 {
        match self {
            Container::Window(w) => w.internal,
            Container::Parent(p) => p.internal,
        }
    }

    /// Returns whether the container is floating within its workspace.
    pub fn is_floating(&self) -> bool {
        match self {
            Container::Window(w) => w.is_floating,
            Container::Parent(p) => p.is_floating,
        }
    }

    /// Get the parent container of this container.
    ///
    /// Returns `None` if the container is a root (has no parent).
    pub fn parent(&self) -> Option<Container> {
        let mut parent = std::mem::MaybeUninit::<crate::bindings::miracle_container_t>::uninit();

        unsafe {
            let result = miracle_container_get_parent(self.id() as i64, parent.as_mut_ptr() as i32);

            if result != 0 {
                return None;
            }

            Some(Container::from(parent.assume_init()))
        }
    }
}

impl From<bindings::miracle_container_t> for Container {
    fn from(value: bindings::miracle_container_t) -> Self {
        match value.type_ {
            1 => Container::Parent(ParentContainer {
                is_floating: value.is_floating != 0,
                layout_scheme: LayoutScheme::try_from(value.layout_scheme).unwrap_or_default(),
                num_children: value.num_child_containers,
                internal: value.internal,
            }),
            _ => Container::Window(WindowContainer {
                is_floating: value.is_floating != 0,
                internal: value.internal,
            }),
        }
    }
}
