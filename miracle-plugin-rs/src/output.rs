use super::bindings;
use super::core::{Point, Size};
use super::host::*;
use super::workspace::*;

/// Represents a physical or logical display.
///
/// Each output hosts one or more [`crate::workspace::Workspace`]s. Use [`Output::workspaces`]
/// to get all workspaces on this output, or [`Output::workspace`] to retrieve one by index.
#[derive(Debug, Clone)]
pub struct Output {
    /// The position of the output.
    pub position: Point,
    /// The size of the output.
    pub size: Size,
    /// The name of the output.
    pub name: String,
    /// Whether this is the primary output.
    pub is_primary: bool,
    /// The number of workspaces on this output.
    pub num_workspaces: u32,
    /// Internal pointer for C interop.
    internal: u64,
}

impl Output {
    /// Returns the opaque internal ID used to refer to this output across API calls.
    pub fn id(&self) -> u64 {
        self.internal
    }

    #[doc(hidden)]
    pub fn from_c_with_name(value: &bindings::miracle_output_t, name: String) -> Self {
        Self {
            position: value.position.into(),
            size: value.size.into(),
            name,
            is_primary: value.is_primary != 0,
            num_workspaces: value.num_workspaces,
            internal: value.internal,
        }
    }

    /// Returns all workspaces hosted by this output.
    pub fn workspaces(&self) -> Vec<Workspace> {
        (0..self.num_workspaces)
            .filter_map(|i| self.workspace(i))
            .collect()
    }

    /// Returns the workspace at `index`, or `None` if out of bounds.
    pub fn workspace(&self, index: u32) -> Option<Workspace> {
        if index >= self.num_workspaces {
            return None;
        }

        const NAME_BUF_LEN: usize = 256;
        let mut workspace = std::mem::MaybeUninit::<crate::bindings::miracle_workspace_t>::uninit();
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let result = miracle_output_get_workspace(
                self.internal as i64,
                index,
                workspace.as_mut_ptr() as i32,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if result != 0 {
                return None;
            }

            let workspace = workspace.assume_init();

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
