# Sticky Keys
Sticky keys is an accessibility feature that enables users to type a keyboard
shortcut one key at a time instead of having to hold down all modifier keys at
once. For example, a user who wants to move the focused window to workspace
two could click `Super` then `Shift` and finally `2` in order to move the window.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

sticky_keys:
    enabled: true
    should_disable_if_two_keys_are_pressed_together: false
```

## Schema

```yaml
sticky_keys:
  enabled?: <boolean>
  should_disable_if_two_keys_are_pressed_together?: <boolean>
```

## Properties

### `enabled`

:   **type:** Boolean  
    **Default:** `false`

    Whether or not sticky keys is enabled.

### `should_disable_if_two_keys_are_pressed_together`

:   **type:** Boolean  
    **Default:** `true`

    When set to `true`, depressing two modifier keys simultaneously will result in sticky keys being temporarily disabled until all keys are released.

## Default
```yaml
sticky_keys:
  enabled: false
  should_disable_if_two_keys_are_pressed_together: true
```
