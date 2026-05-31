# Action Key
The **action key** defines the primary key that will initiate a command in the compositor.
[Default Keybinds](default_keybinds.md) and [Custom Actions](custom_actions.md) can refer
to this key using the word `primary` as a modifier key. Please see those configuration options
for more info.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

action_key: shift
```

## Schema

```yaml
action_key: <modifier>
```

## Properties

### `action_key`

:   <small>required</small> **type:** Modifier key name

    A modifier string that defines the primary action key. Available options:
    
    | Name | Description |
    | ---- | ----------- |
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

## Default
```yaml
action_key: meta
```

This is the `super` or `windows` key.
