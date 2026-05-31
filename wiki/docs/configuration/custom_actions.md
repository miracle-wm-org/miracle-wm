# Custom Actions
The user may define a list of **custom actions**, which are shell commands
bound to a specific key combination. These actions always have preference
over those defined in [Default Keybinds](#default_keybinds.md) when they are
bound to the same key combination.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

custom_actions:           # Set meta + D to open wofi
  - command: wofi --show=drun
    action: down
    modifiers:
      - primary
    key: d
```

## Schema

A list of custom keybind actions:

```yaml
custom_actions:
  - command: <string>
    action: <up|down|repeat|modifiers>
    modifiers: <Modifier[]>
    key: <KeysymName>
```

## Properties

### `command`

:   <small>required</small> **type:** String

    The shell command to execute when the keybind is triggered.

### `action`

:   <small>required</small> **type:** `up` | `down` | `repeat` | `modifiers`

    The key action that triggers the command:
    
    - `up` — Triggered when the key is released
    - `down` — Triggered when the key is pressed
    - `repeat` — Triggered repeatedly while the key is held
    - `modifiers` — Triggered when modifiers change

### `modifiers`

:   <small>required</small> **type:** List of modifier keys

    Modifier keys that must be held for the command to execute. Available modifiers:
    
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

:   <small>required</small> **type:** KeysymName

    Name of the XKB keysym that the action responds to. See the [xkbcommon keysym list](https://xkbcommon.org/doc/current/xbkcommon-keysyms_8h.html) for available names (e.g., `Return`, `z`, `Up`).

    For shifted characters, use the shifted keysym directly instead of combining a lowercase key with the `shift` modifier. For example, use `Q` instead of `q` + `shift`, and use `exclam` instead of `1` + `shift`.

## Default
```yaml
custom_actions: []
```
