use super::bindings;
use super::container::*;
use super::core::Rectangle;
use super::host::*;
use super::output::*;

/// Represents one workspace on an output.
///
/// Each workspace hosts a tree of [`crate::container::Container`]s. Use [`Workspace::trees`]
/// to get the top-level containers, and [`Workspace::output`] to find the display this
/// workspace is hosted on.
#[derive(Debug, Clone)]
pub struct Workspace {
    /// The workspace number (if set).
    pub number: Option<u32>,
    /// The workspace name (if set).
    pub name: Option<String>,
    /// The current area of the workspace.
    pub rectangle: Rectangle,
    internal: u64,
}

impl Workspace {
    /// Returns the opaque internal ID used to refer to this workspace across API calls.
    pub fn id(&self) -> u64 {
        self.internal
    }

    #[doc(hidden)]
    pub unsafe fn from_c_with_name(value: &bindings::miracle_workspace_t, name: String) -> Self {
        Self {
            number: if value.has_number != 0 {
                Some(value.number)
            } else {
                None
            },
            name: if value.has_name != 0 {
                Some(name)
            } else {
                None
            },
            internal: value.internal,
            rectangle: Rectangle {
                x: value.position.x,
                y: value.position.y,
                width: value.size.w,
                height: value.size.h,
            },
        }
    }

    /// Get the output that this workspace is on.
    pub fn output(&self) -> Option<Output> {
        const NAME_BUF_LEN: usize = 256;
        let mut output = std::mem::MaybeUninit::<crate::bindings::miracle_output_t>::uninit();
        let mut name_buf: [u8; NAME_BUF_LEN] = [0; NAME_BUF_LEN];

        unsafe {
            let result = miracle_workspace_get_output(
                self.internal as i64,
                output.as_mut_ptr() as i32,
                name_buf.as_mut_ptr() as i32,
                NAME_BUF_LEN as i32,
            );

            if result != 0 {
                return None;
            }

            let output = output.assume_init();

            // Find the null terminator to get the actual string length
            let name_len = name_buf
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(NAME_BUF_LEN);
            let name = String::from_utf8_lossy(&name_buf[..name_len]).into_owned();

            Some(Output::from_c_with_name(&output, name))
        }
    }

    /// Get a tree from the workspace by index.
    ///
    /// Returns `None` if the index is out of bounds.
    pub fn tree(&self) -> Option<Container> {
        let mut container = std::mem::MaybeUninit::<crate::bindings::miracle_container_t>::uninit();
        unsafe {
            let result = miracle_workspace_get_tree(
                self.internal as i64,
                container.as_mut_ptr() as i32,
            );

            if result != 0 {
                return None;
            }

            let container = container.assume_init();
            Some(Container::from(container))
        }
    }
}
