> This user manual will keep up to date with the current state of the project.
> Please note that the information in here is likely to change with future release.

# Index
- [Built-in Keybinds](#built-in-keybinds)
- [Pointer Behavior](#pointer-behavior)
- [Configuration File](#configuration-file)
  - [Location](#location)
  - [Types](#types)
  - [YAML Definition](#yaml-definition)
  - [Example](#example)
- [Display Configuration](#display-configuration)

# Built-in Keybinds
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
- `meta + f`: Toggle fullscreen on the window
- `meta + [0-9]`: Move to workspace *n*
- `meta + shift + [0-9]`: Move active window to workspace *n*
- `meta + space`: Toggle selected window as "floating"
- `meta + shift + p`: Toggle whether a floating window is pinned to a workspace or not

# Pointer Behavior

**For tiling windows**:
- Hovering over a window will select the window
- Windows may be minimized, maximized, or removed using the toolbar icons
- Window CANNOT be resized or moved with the pointer

**For floating windows**:
- Windows may be minimized, maximized, removed, resized, or moved using traditional pointer behavior
- `meta + hold left click` to move a floating window around on the screen
- Floating windows are *always* displayed above tiled windows

# Configuration File

## Location
The configuration file will be written blank the first time that you start the compositor. The file is named `miracle-wm.yaml`
and it is written to your config directory, most likely at `~/.config/miracle-wm.yaml`.

## Types
First, let's define some reoccurring data types in the configuration file:

  ```c++
// Represents a modifier to be used in conjunction with another key press (e.g. Ctrl + Alt + Delete; Ctrl and Alt would be modifiers)
typedef ModifierKey =
    "alt" | "alt_left" | "alt_right" | "shift" | "shift_left" | "shift_right" | "sym" | "function" | "ctrl"
    | "ctrl_left" | "ctrl_right" | "meta" | "meta_left" | "meta_right" | "caps_lock" | "num_lock" | "scroll_lock"
    | "primary"; // "primary" means that the action should take whatever is defined by the action_key

// Represents an internal action that the user can override
struct DefaultActionOverride
{
    // Name of the action to override
    name: "terminal" | "request_vertical" | "request_horizontal" | "toggle_resize" | "move_up" | "move_down"
      | "move_left" | "move_right" | "select_up" | "select_down" | "select_left" | "select_right" 
      | "quit_active_window" | "quit_compositor" | "fullscreen" | "select_workspace_1" | "select_workspace_2"
      | "select_workspace_3" | "select_workspace_4" | "select_workspace_5" | "select_workspace_6"
      | "select_workspace_7" | "select_workspace_8" | "select_workspace_9" | "select_workspace_0"
      | "move_to_workspace_1" | "move_to_workspace_2" | "move_to_workspace_3" | "move_to_workspace_4"
      | "move_to_workspace_5" | "move_to_workspace_6" | "move_to_workspace_7" | "move_to_workspace_8"
      | "move_to_workspace_9" | "move_to_workspace_0" | "toggle_floating" | "toggle_pinned_to_workspace"
      
    action: "up" | "down" | "repeat" | "modifiers"; // Action will fire based on this key event
    modifiers: Modifier[]; // Modifiers required for the action to trigger
    
    // Name of the keycode that the action should respond to.
    // See https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
    // for the list of available keycodes (e.g. KEY_ENTER, KEY_Z, etc.)
    key: KeyCodeName;
};

// Represents a custom action that a user can bind to a key combination
struct CustomAction
{
    command: string // Action to execute
    action: "up" | "down" | "repeat" | "modifiers"; // Action will fire based on this key event
    modifiers: Modifier[]; // Modifiers required for the action to trigger
    // Name of the keycode that the action should respond to.
    // See https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
    // for the list of available keycodes (e.g. KEY_ENTER, KEY_Z, etc.)
    key: KeyCodeName;
};

// Represents an app that will be executed at startup
struct StartupApp
{
    command: string;
    restart_on_death: bool; // If true the application will restart whenever it dies
                            // Note that applications with restart_on_death set to true
                            // will restart when the configuration changes.
};

struct Vector2
{
    int x;
    int y;
};

struct EnvironmentVariable
{
    string key;
    string value;
};
```

## YAML Definition
With those types defined, the following table defines the allowed key/value pairs:

| Key                      | Default                         | Type                      | Description                                                                                                                                                                                                                                                            |
|--------------------------|---------------------------------|---------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| action_key               | `meta`                          | `Modifier`                | The default key that is used to initate any action.                                                                                                                                                                                                                    |
| default_action_overrides | `[]`                            | `DefaultActionOverride[]` | A list overrides to apply to built-in actions. Actions may be overridden more than once and will respond to multiple key combinations as a result. Defining at least one override disables the default action defined in [Default Key Commands](#default-key-commands) |
| custom_actions           | []                              | `CustomAction[]`          | A list of custom applications that I user can execute. These actions always have precedence over built-in actions.                                                                                                                                                     |
| inner_gaps               | `x: 10, y: 10`                  | `Vector2`                 | Size of the gaps in pixels between windows                                                                                                                                                                                                                             |                                                                                                                                                                                                      |
| outer_gaps               | `x: 10, y: 10`                  | `Vector2`                 | Size of the gap between the window group and the edge of the screen in pixels                                                                                                                                                                                          |                                                                                                                                                                                                                 |
| startup_apps             | []                              | `StartupApp[]`            | List of applications to be started when the compositor starts                                                                                                                                                                                                          |
| terminal                 | `"miracle-mw-sensible-terminal"` | `String`                  | The command used when launching a terminal. Defaults to a script that attempts to find a suitable terminal, based on `i3-sensible-terminal`                                                                                                                            |
| resize_jump              | 50                              | `int`                     | Jump in pixels for each resize request.                                                                                                                                                                                                                                |
| environment_variables    | []                              | `EnvironmentVariable[]`   | Environment variables to be set by the compositor on startup |

## Example
```yaml
action_key: alt           # Set the primary action key to alt
terminal: konsole
default_action_overrides:
  - name: terminal        # Override the "terminal" keybind to execute with "Ctrl + Shift + Enter"
    action: down
    modifiers:
      - ctrl
      - shift
    key: KEY_ENTER

custom_actions:           # Set meta + D to open wofi
  - command: wofi --show=drun
    action: down
    modifiers:
      - primary
    key: KEY_D

inner_gaps:
  x: 20
  y: 20

outer_gaps:
  x: 100
  y: 100
  
startup_apps:
  - command: waybar
    restart_on_death: true
  - command: swaybg -i /path/to/my/image
    restart_on_death: true
```

# Display Configuration
For the time being, you may specify the command line option for Mir:
```
--display-config=static=/path/to/a/display/file.yaml
```

Eventually this will move out to the configuration once https://github.com/mattkae/miracle-wm/issues/93 is implemented.