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

:   <small>optional</small> **type:** Integer

    Border width in pixels. Defaults to `2`.

### `color`

:   <small>optional</small> **type:** Color

    Border color for non-focused tiles. Defaults to `0x55595cff`. Can be specified as:
    
    - Hex color (rgba): `0xffff0000`
    - YAML object with r, g, b, a properties (0-255 each)

### `focus_color`

:   <small>optional</small> **type:** Color

    Border color for focused tiles. Defaults to `0xfbda25ff`. Can be specified as:
    
    - Hex color (rgba): `0xff00ff00`
    - YAML object with r, g, b, a properties (0-255 each)

### `radius`

:   <small>optional</small> **type:** Integer

    Border corner radius in pixels. Defaults to `8`.

## Default

Borders are enabled by default. Omit the `border` section entirely to keep these values,
or set `size: 0` to disable borders.

```yaml
border:
  size: 2
  color: 0x55595cff
  focus_color: 0xfbda25ff
  radius: 8
```
