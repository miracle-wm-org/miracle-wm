# Mouse Configuration
Configure the mouse input device.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

# Override the mouse configuration to change the handedness to left
# and slow down the pointer speed
mouse:
    handedness: left
    acceleration_bias: -0.5
```

## Schema

```yaml
mouse:
  handedness?: <left|right>
  vscroll_speed?: <double>
  hscroll_speed?: <double>
  acceleration_bias?: <double>
  acceleration?: <none|adaptive>
```

## Properties

### `handedness`

:   **type:** `left` | `right`  
    **Default:** `right`

    The handedness of the mouse. Setting this will change which button is considered 'primary'.

### `vscroll_speed`

:   **type:** Double  
    **Default:** `1.0`

    The vertical scroll speed, if supported.

### `hscroll_speed`

:   **type:** Double  
    **Default:** `1.0`

    The horizontal scroll speed, if supported.

### `acceleration_bias`

:   **type:** Double (range: -1 to 1)  
    **Default:** `0.0`

    The acceleration bias of your mouse. A value of -1 will be slow, and a value of 1 will be fast.

### `acceleration`

:   **type:** `none` | `adaptive`  
    **Default:** `none`

    The acceleration type of your mouse.

## Default
```yaml
mouse:
  handedness: right
  vscroll_speed: 1.0
  hscroll_speed: 1.0
  acceleration_bias: 0.0
  acceleration: none
```
