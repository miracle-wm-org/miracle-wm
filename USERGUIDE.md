> This user manual will keep up to date with the current state of the project.
> Please note that the information in here is likely to change with future release.

# Default Key Commands
- `meta + enter`: Open new terminal
- `meta + h`: Switch current lane to horizontal layout mode
- `meta + v`: Switch current lane to vertical layout mode
- `meta + shift + up`: Move selected window up in the tree
- `meta + shift + down`: Move selected window down in the tree
- `meta + shift + left`: Move selected window left in the tree
- `meta + shift + right`: Move selected window right in the tree
- `meta + up`: Select the window above the currently selected window
- `meta + down`: Select the window below the currently selected window
- `meta + left`: Select the window to the left of the currently selected window
- `meta + right`: Select the window to the right of the currently selected window
- `meta + r`: Toggle resize mode on the active node
  - `meta + left`: Resize to the left
  - `meta + right`: Resize to the right
  - `meta + up`: Resize upward
  - `meta + down`: Resize downward
- `meta + shift + q`: Quit the selected application
- `meta + shift + e`: Close the compositor

# Pointer Behavior
- Hovering over a window will select the window
- Windows may be minimized, maximized, or removed using the toolbar icons
- Window CANNOT be resized or moved with the pointer

# Configuration File
The configuration file will be written blank the first time that you start the compositor. The file is named `miracle-wm.yaml`
and it is written to your config directory, most likely at `~/.config/miracle-wm.yaml`.

## Data Types
First, let's define some reoccurring data types in the configuration file:

- `ModifierKey`: represents a modifier to be used in conjunction with another key press (e.g. Ctrl + Alt + Delete; Ctrl and Alt would be modifiers)

  ```c++
  enum ModifierKey: String
  {
      alt,
      alt_left,
      alt_right,
      shift,
      shift_left,
      shift_right,
      sym,
      function,
      ctrl,
      ctrl_left,
      ctrl_right,
      meta,
      meta_left,
      meta_right,
      caps_lock,
      num_lock,
      scroll_lock,
      primary // denotes that the action should take whatever is defined by the action_key
  };
  ```

- `DefaultActionOverride`: defines a keybind override of an action that is built-in to the compositor.
  ```c++
  struct DefaultActionOverride
  {
      // Name of the action to override
      name: "terminal" | "request_vertical" | "request_horizontal" | "toggle_resize" | "move_up" | "move_down" | "move_left" | "move_right" | "select_up" | "select_down" | "select_left" | "select_right" | "quit_active_window" | "quit_compositor";
  
      // Action will fire based on this key event
      action: "up" | "down" | "repeat" | "modifiers";
      
      // Modifiers required for the action to trigger
      modifiers: Modifier[];
      
      // Name of the keycode that the action should respond to.
      // See https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
      // for the list of available keycodes (e.g. KEY_ENTER, KEY_Z, etc.)
      key: KeyCodeName;
  };
  ```

## Configuration Definition
With those types defined, the following table defines the allowed key/value pairs:

| Key                      | Default | Type              | Description                                                                                                                                                                                                                                                            |
|--------------------------|---------|-------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| action_key               | `meta`    | `Modifier`          | The default key that is used to initate any action.                                                                                                                                                                                                                    |
| default_action_overrides | `[]`      | `DefaultActionOverride[]` | A list overrides to apply to built-in actions. Actions may be overridden more than once and will respond to multiple key combinations as a result. Defining at least one override disables the default action defined in [Default Key Commands](#default-key-commands) |
| gap_size_x               | 10 | int | Size of the gaps in pixels horizontally between windows          |                                                                                                                                                                                                      |
| gap_size_y               | 10 | int | Size of the gaps in pixels vertically between windows |                                                                                                                                                                                                                 |

## Example Configuration
```yaml
action_key: alt           # Set the primary action key to alt
default_action_overrides:
  - name: terminal        # Override the "terminal" keybind to execute with "Ctrl + Shift + Enter"
    action: down
    modifiers:
      - ctrl
      - shift
    key: KEY_ENTER
gap_size_x: 20
gap_size_y: 20
```