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
