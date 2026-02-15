use super::bindings;
use super::host::*;
use super::window::*;

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
    pub internal: u64,
}

impl Container {
    /// Get a child container from the parent container by index.
    ///
    /// Returns `None` if the index is out of bounds or if the container
    /// is not of type `Parent`.
    pub fn child_at(&self, index: u32) -> Option<Container> {
        if self.container_type != ContainerType::Parent || index >= self.num_children {
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

    /// Get all children from a parent container.
    ///
    /// Returns an empty vector if the container is not of type `Parent`.
    pub fn get_children(&self) -> Vec<Container> {
        if self.container_type != ContainerType::Parent {
            return Vec::new();
        }
        (0..self.num_children)
            .filter_map(|i| self.child_at(i))
            .collect()
    }

    /// Get the window info from a window container.
    ///
    /// Returns `None` if the container is not of type `Window`.
    pub fn window(&self) -> Option<WindowInfo> {
        if self.container_type != ContainerType::Window {
            return None;
        }

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

            // Find the null terminator to get the actual string length
            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(WindowInfo::from_c_with_name(&window, name))
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
