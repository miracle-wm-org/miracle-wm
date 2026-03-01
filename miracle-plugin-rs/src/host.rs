unsafe extern "C" {
    /// Retrieve the application info for a given window.
    /// Returns the internal ID on success, or -1 if the buffer is too small.
    pub fn miracle_window_info_get_application(
        window_info_internal: i64,
        name_buf_ptr: i32,
        name_buf_len: i32,
    ) -> i64;

    /// Retrieve the workspace that a window is on.
    pub fn miracle_window_info_get_workspace(
        window_info_internal: i64,
        out_ptr: i32,
        name_buf: i32,
        name_buf_len: i32,
    ) -> i32;

    /// Retrieve the output that a workspace is on.
    pub fn miracle_workspace_get_output(
        workspace_internal: i64,
        out_ptr: i32,
        name_buf: i32,
        name_buf_len: i32,
    ) -> i32;

    /// Retrieve the number of outputs.
    pub fn miracle_num_outputs() -> u32;

    /// Retrieve an output by index.
    pub fn miracle_get_output_at(index: u32, out_ptr: i32, name_buf: i32, name_buf_len: i32)
    -> i32;

    /// Retrieve a tree from a workspace by index.
    pub fn miracle_workspace_get_tree(workspace_internal: i64, index: u32, out_ptr: i32) -> i32;

    /// Retrieve a child container from a parent container by index.
    pub fn miracle_container_get_child_at(container_internal: i64, index: u32, out_ptr: i32)
    -> i32;

    /// Retrieve the window info from a window container.
    pub fn miracle_container_get_window(
        container_internal: i64,
        out_ptr: i32,
        name_buf: i32,
        name_buf_len: i32,
    ) -> i32;

    /// Retrieve the workspace on the output by index.
    pub unsafe fn miracle_output_get_workspace(
        output_internal: i64,
        index: u32,
        out_ptr: i32,
        name_buf: i32,
        name_buf_len: i32,
    ) -> i32;

    /// Retrieve the currently active workspace on the focused output.
    pub fn miracle_get_active_workspace(out_ptr: i32, name_buf: i32, name_buf_len: i32) -> i32;

    /// Retrieve the number of windows managed by the given plugin handle.
    pub fn miracle_num_managed_windows(plugin_handle: u32) -> u32;

    /// Retrieve a managed window by index.
    /// Returns 0 on success, non-zero on failure.
    pub fn miracle_get_managed_window_at(
        plugin_handle: u32,
        index: u32,
        out_ptr: i32,
        name_buf: i32,
        name_buf_len: i32,
    ) -> i32;

    /// Set the state of a managed window.
    /// Returns 0 on success, -1 on error.
    pub fn miracle_window_set_state(window_internal: i64, state: i32) -> i32;

    /// Move a managed window to a different workspace.
    /// Returns 0 on success, -1 on error.
    pub fn miracle_window_set_workspace(window_internal: i64, workspace_internal: i64) -> i32;

    /// Resize a managed window.
    /// Returns 0 on success, -1 on error.
    pub fn miracle_window_set_size(window_internal: i64, width: i32, height: i32) -> i32;

    /// Set the 4x4 column-major transform matrix of a managed window.
    /// `transform_ptr` is a WASM linear memory offset pointing to 16 contiguous f32 values.
    /// Returns 0 on success, -1 on error.
    pub fn miracle_window_set_transform(window_internal: i64, transform_ptr: i32) -> i32;

    /// Set the alpha (opacity) of a managed window.
    /// `alpha_ptr` is a WASM linear memory offset pointing to a single f32 value.
    /// Returns 0 on success, -1 on error.
    pub fn miracle_window_set_alpha(window_internal: i64, alpha_ptr: i32) -> i32;

    /// Request a workspace by optional number and/or name.
    ///
    /// If a workspace with the given number or name already exists, it is returned.
    /// Otherwise, a new workspace is created on the focused output.
    pub fn miracle_request_workspace(
        has_number: i32,
        number: i32,
        name_in_ptr: i32,
        name_in_len: i32,
        out_workspace_ptr: i32,
        out_name_buf_ptr: i32,
        out_name_buf_len: i32,
        focus: i32,
    ) -> i32;
}
