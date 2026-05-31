# Outer Gaps
Gaps between the window tiles and the edge of the screen.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

outer_gaps:
    x: 50  # 50px between the tiling grid and the edge of the output, horizontally
    y: 100 # 100px between the tiling grid and the edge of the output, vertically 
```

## Schema

```yaml
outer_gaps:
  x: <int>
  y: <int>
```

## Properties

### `x`

:   <small>required</small> **type:** Integer  
    **Default:** `10`

    Horizontal gap between the tiling grid and the edge of the output in pixels.

### `y`

:   <small>required</small> **type:** Integer  
    **Default:** `10`

    Vertical gap between the tiling grid and the edge of the output in pixels.

## Default
```yaml
outer_gaps:
  x: 10
  y: 10
```
