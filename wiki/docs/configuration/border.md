# Border
Specify a border for non-focused and focused tiles.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

border:
  size: 2
  color: 0xffff0000
  focus_color:
    r: 0
    g: 255
    b: 0
    a: 255
  radius: 16
```

## Schema

```yaml
border:
  size: <int>
  color: <Color>
  focus_color: <Color>
  radius: <int>
```

## Properties

### `size`

:   <small>required</small> **type:** Integer

    Border width in pixels.

### `color`

:   <small>required</small> **type:** Color

    Border color for non-focused tiles. Can be specified as:
    
    - Hex color (rgba): `0xffff0000`
    - YAML object with r, g, b, a properties (0-255 each)

### `focus_color`

:   <small>required</small> **type:** Color

    Border color for focused tiles. Can be specified as:
    
    - Hex color (rgba): `0xff00ff00`
    - YAML object with r, g, b, a properties (0-255 each)

### `radius`

:   <small>required</small> **type:** Integer

    Border corner radius in pixels.

## Default
```yaml
border:
  size: 0
  color: 0x00000000
  focus_color: 0x00000000
  radius: 0
```
