# Default Keybinds

The compositors comes ships with a number of built-in commands. By default, these
commands are bound to preset keybinds, but the user may change them to whatever
they want. These commands may be overridden more than once and will respond to multiple key combinations as a result. Defining at least one override disables the default action.

The default commands defined in the compositor are described in this table:

| Name                         | Description                                                                            | Keybind                             |
| ---------------------------- | -------------------------------------------------------------------------------------- | ----------------------------------- |
| `terminal`                   | Opens a new terminal                                                                   | `❖ Super + Enter`                   |
| `reload_config`              | Reload the configuration                                                               | `❖ Super + Shift + R`               |
| `request_vertical`           | Requests that the current window layout future windows vertially                       | `❖ Super + v`                       |
| `request_horizontal`         | Requests that the current window layout future windows horizontaly                     | `❖ Super + h`                       |
| `select_up`                  | Select the window above currently selected window                                      | `❖ Super + ↑`                       |
| `select_down`                | Select the window above currently selected window                                      | `❖ Super + ↓`                       |
| `select_left`                | Select the window above currently selected window                                      | `❖ Super + ←`                       |
| `select_right`               | Select the window above currently selected window                                      | `❖ Super + →`                       |
| `move_up`                    | Move the currently selected window upwards                                             | `❖ Super + ⇧ Shift + ↑`             |
| `move_down`                  | Move the currently selected window downwards                                           | `❖ Super + ⇧ Shift + ↓`             |
| `move_left`                  | Move the currently selected window to the left                                         | `❖ Super + ⇧ Shift + ←`             |
| `move_right`                 | Move the currently selected window to the right                                        | `❖ Super + ⇧ Shift + →`             |
| `toggle_resize`              | Toggle resize mode on the active window                                                | `❖ Super + r`                       |
| `resize_up`                  | When resize mode is toggled on, this will decrease the size of the window vertically   | `❖ Super + ↑` (only in resize mode) |
| `resize_down`                | When resize mode is toggled on, this will increase the size of the window vertically   | `❖ Super + ↓` (only in resize mode) |
| `resize_left`                | When resize mode is toggled on, this will decrease the size of the window horizontally | `❖ Super + ←` (only in resize mode) |
| `resize_down`                | When resize mode is toggled on, this will increase the size of the window horizontally | `❖ Super + →` (only in resize mode) |
| `fullscreen`                 | Fullscreen the currently selected window                                               | `❖ Super + f`                       |
| `quit_active_window`         | Close the currently selected window                                                    | `❖ Super + ⇧ Shift + Q`             |
| `quit_compostior`            | Exit the compositor                                                                    | `❖ Super + ⇧ Shift + E`             |
| `select_workspace_[0-9]`     | Select the workspace between 0-9                                                       | `❖ Super + [0-9]`                   |
| `select_workspace_[0-9]`     | Move the currently selected window to the workspace between 0-9                        | `❖ Super + ⇧ Shift + [0-9]`         |
| `toggle_floating`            | Toggle whether or not the currently selected window is floating                        | `❖ Super + Space`                   |
| `toggle_pinned_to_workspace` | Toggle whether a floating window is pinned outside of a workspace or not               | `❖ Super + ⇧ Shift + P`             |
| `toggle_tabbing`             | Toggle whether or not the currently selected container is in a tabbed layout mode      | `❖ Super + W`                       |
| `toggle_stacking`            | Toggle whether or not the currently selected container is in a stacking layout mode    | `❖ Super + S`                       |
| `magnifier_on`               | Toggle whether the magnifier accessibility feature is enabled                          | `❖ Super + Shift + =`               |
| `magnifier_off`              | Enable the magnifier accessibility feature                                             | `❖ Super + Esc`                     |
| `magnifier_off`              | Disable the magnifier accessibility feature                                            | `❖ Super + Esc`                     |
| `magnifier_increase_size`    | Increase the area of the magnifier                                                     | `❖ Super + Shift + =`               |
| `magnifier_decrease_size`    | Decrease the area of the magnifier                                                     | `❖ Super + Shift + -`               |
| `magnifier_increase_scale`   | Increase the scale of the magnifier                                                    | `❖ Super + =`                       |
| `magnifier_decrease_scale`   | Decrease the scale of the magnifier                                                    | `❖ Super + -`                       |

## Example

```yaml
# ~/.config/miracle-wm/config.yaml

default_action_overrides:
  - name: terminal # Override the "terminal" keybind to execute with "Ctrl + Shift + Enter"
    action: down
    modifiers:
      - ctrl
      - shift
    key: Return
```

## Schema

A list of keybind override definitions:

```yaml
default_action_overrides:
  - name: <string>
    action: <up|down|repeat|modifiers>
    modifiers: <Modifier[]>
    key: <KeysymName>
```

## Properties

### `name`

: <small>required</small> **type:** String

    The name of the default action to override (see table above).

### `action`

: <small>required</small> **type:** `up` | `down` | `repeat` | `modifiers`

    The key action that triggers the command:

    - `up` — Triggered when the key is released
    - `down` — Triggered when the key is pressed
    - `repeat` — Triggered repeatedly while the key is held
    - `modifiers` — Triggered when modifiers change

### `modifiers`

: <small>required</small> **type:** List of modifier keys

    Modifier keys that must be held for the action to execute. Available modifiers:

    | Name | Description |
    | ---- | ----------- |
    | `primary` | the key defined by the [Action Key](action_key.md) |
    | `alt` | Any alt key |
    | `alt_left` | The left alt key only |
    | `alt_right` | The right alt key only |
    | `shift` | Any shift key |
    | `shift_left` | The left shift key only |
    | `shift_right` | The right shift key only |
    | `ctrl` | Any ctrl key |
    | `ctrl_left` | The left ctrl key only |
    | `ctrl_right` | The right ctrl key only |
    | `meta` | The `super` or `windows` key |
    | `meta_left` | The left `super` or `windows` key only |
    | `meta_right` | The right `super` or `windows` key only |
    | `sym` | The sym key |
    | `function` | The `fn` key |
    | `caps_lock` | The caps lock key |
    | `num_lock` | The num lock key |
    | `scroll_lock` | The scroll lock key |

### `key`

: <small>required</small> **type:** KeysymName

    Name of the XKB keysym that the action responds to. See the [xkbcommon keysym list](https://xkbcommon.org/doc/current/xbkcommon-keysyms_8h.html) for available names (e.g., `Return`, `z`, `Up`).

    For shifted characters, use the shifted keysym directly instead of combining a lowercase key with the `shift` modifier. For example, use `Q` instead of `q` + `shift`, and use `exclam` instead of `1` + `shift`.

## Default

```yaml
default_action_overrides: []
```
