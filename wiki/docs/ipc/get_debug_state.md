# GET_DEBUG_STATE (200)
Retrieves a snapshot of debugging information: the cursor position, the window directly
under the cursor, and a flat list of every window across every output and workspace. This is
a miracle-specific message used by the bundled [debug overlay](commands/debug.md), but it can
be consumed by any IPC client.

## Payload
Empty

## Reply
```json
{
    // The current cursor position, in global output coordinates.
    "cursor": { "x": integer, "y": integer },

    // The `debug_id` of the window directly under the cursor, or -1 if none.
    "window_under_cursor": integer,

    // Every window, regardless of which workspace/output it is on.
    "windows": [
        {
            // Stable container id used by the debug tooling. Matches
            // `window_under_cursor`.
            "debug_id": integer,

            // The window's logical position and size, in global output
            // coordinates.
            "rect": { "x": integer, "y": integer, "width": integer, "height": integer },

            // The visible (clip) area, expressed relative to `rect`.
            "window_rect": { "x": integer, "y": integer, "width": integer, "height": integer },

            // Global bounding box of the surface's input area (queried from Mir),
            // useful for debugging input bugs.
            "input_bounds": { "x": integer, "y": integer, "width": integer, "height": integer },

            // The individual rectangles that accept pointer input, in global
            // coordinates. An empty array means the whole surface accepts input
            // (i.e. equal to `input_bounds`).
            "input_region": [
                { "x": integer, "y": integer, "width": integer, "height": integer }
            ],

            // Whether the window is focused / currently visible.
            "focused": boolean,
            "visible": boolean,

            // Application id, name, scratchpad state, fullscreen flag, ...
            // (the same per-window fields emitted by GET_TREE).
            "app_id": string,
            "name": string,

            // Annotations added for debugging.
            "output": string,
            "output_focused": boolean,
            "workspace_id": integer,
            "workspace_name": string
        }
        // ...
    ]
}
```

## Example
```sh
miraclemsg -t get_debug_state
```

## Notes
- Introduced in v0.10.0.
- `rect` and `window_rect` mirror the fields emitted by [GET_TREE](get_tree.md); `rect` is in
  global output coordinates and `window_rect` is the clip area relative to `rect`.
