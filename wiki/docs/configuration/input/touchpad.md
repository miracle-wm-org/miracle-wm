# Touchpad
Describes the touchpad configuration

## Example
```yaml
touchpad:
    disable_while_typing: true
    tap_to_click: true
    middle_mouse_button_emulation: true
    vscroll_speed: 1.2
    acceleration_bias: -0.1
    click_mode: none
    scroll_mode: two_finger_scroll
```

## Schema

```yaml
touchpad:
  disable_while_typing?: <boolean>
  disable_with_external_mouse?: <boolean>
  acceleration_bias?: <float>
  vscroll_speed?: <float>
  hscroll_speed?: <float>
  tap_to_click?: <boolean>
  middle_mouse_button_emulation?: <boolean>
  click_mode?: <none|area_to_click|finger_count>
  scroll_mode?: <none|two_finger_scroll|edge_scroll|button_down_scroll>
```

## Properties

### `disable_while_typing`

:   **type:** Boolean  
    **Default:** `false`

    Whether or not the touchpad is disabled while typing.

### `disable_with_external_mouse`

:   **type:** Boolean  
    **Default:** `false`

    Whether or not to disable the touchpad when an external mouse is connected.

### `acceleration_bias`

:   **type:** Float (range: -1 to 1)  
    **Default:** `0.0`

    The acceleration bias of the touchpad.

### `vscroll_speed`

:   **type:** Float  
    **Default:** `1.0`

    The vertical scroll speed of the touchpad.

### `hscroll_speed`

:   **type:** Float  
    **Default:** `1.0`

    The horizontal scroll speed of the touchpad.

### `tap_to_click`

:   **type:** Boolean  
    **Default:** `true`

    Whether tap-to-click is enabled.

### `middle_mouse_button_emulation`

:   **type:** Boolean  
    **Default:** `false`

    Whether middle mouse button emulation is enabled.

### `click_mode`

:   **type:** `none` | `area_to_click` | `finger_count`  
    **Default:** `none`

    The click mode for the touchpad.

### `scroll_mode`

:   **type:** `none` | `two_finger_scroll` | `edge_scroll` | `button_down_scroll`  
    **Default:** `two_finger_scroll`

    The scroll mode for the touchpad.

## Default
```yaml
touchpad:
  disable_while_typing: false
  disable_with_external_mouse: false
  acceleration_bias: 0.0
  vscroll_speed: 1.0
  hscroll_speed: 1.0
  tap_to_click: true
  middle_mouse_button_emulation: false
  click_mode: none
  scroll_mode: two_finger_scroll
```
