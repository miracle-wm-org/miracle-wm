# Background Color

`background_color` is the color that the compositor clears the screen to. It is what you see wherever no window covers the screen.

The background is always fully opaque, so no alpha value is required. If you supply one anyway, it is ignored.

Changes take effect as soon as the configuration is reloaded — no restart is required.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

background_color: 0x1E1E2E
```

## Schema

```yaml
background_color: <SolidColor>
```

## Properties

### `background_color`

:   <small>required</small> **type:** SolidColor

    The color the compositor clears the screen to. Can be specified as:

    - Hex color (rgb): `0x1E1E2E`. The `RRGGBBAA` form (`0x1E1E2EFF`) is also accepted, with the alpha discarded.
    - YAML array of `[r, g, b]` (0-255 each): `[30, 30, 46]`. A fourth entry is accepted and ignored.
    - YAML object with r, g, b properties (0-255 each)

## Default
```yaml
background_color: 0x2E3436
```
