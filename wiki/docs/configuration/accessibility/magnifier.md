# Magnifier
The magnifier accessibility feature allows users to zoom in on an area of the
desktop centered around their mouse cursor.

See the [Default Keybinds](../default_keybinds.md) section for the keybindings
associated with the magnifier.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

magnifier:
    enabled: true
    scale: 2
    scale_increment: 0.25
    width: 500
    height: 500
    size_increment: 25
```

## Schema

```yaml
magnifier:
  enabled?: <boolean>
  scale?: <float>
  scale_increment?: <float>
  width?: <int>
  height?: <int>
  size_increment?: <int>
```

## Properties

### `enabled`

:   **type:** Boolean  
    **Default:** `false`

    Whether or not the magnifier is enabled by default.

### `scale`

:   **type:** Float  
    **Default:** `1.5`

    The scale (zoom level) of the magnifier.

### `scale_increment`

:   **type:** Float  
    **Default:** `0.5`

    When the magnifier is zoomed in or out using the keybinds, this is the increment by which that will happen. The magnifier's zoom is clamped to 1 as a minimum.

### `width`

:   **type:** Integer  
    **Default:** `400`

    The default width of the magnified area in pixels.

### `height`

:   **type:** Integer  
    **Default:** `400`

    The default height of the magnified area in pixels.

### `size_increment`

:   **type:** Integer  
    **Default:** `50`

    When the magnifier's zoomed area increases or decreases in size via the keyboard shortcut, this is the increment by which that will happen. The magnifier's size is clamped to 100x100.

## Default
```yaml
magnifier:
  enabled: false
  scale: 1.5
  scale_increment: 0.5
  width: 400
  height: 400
  size_increment: 50
```
