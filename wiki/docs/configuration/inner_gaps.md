# Inner Gaps
Gaps between windows in pixels.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

inner_gaps:
    x: 50 # 50px between windows horizontally
    y: 20 # 20px between windows vertically
```

## Schema

```yaml
inner_gaps:
  x: <int>
  y: <int>
```

## Properties

### `x`

:   <small>required</small> **type:** Integer  
    **Default:** `10`

    Horizontal gap between windows in pixels.

### `y`

:   <small>required</small> **type:** Integer  
    **Default:** `10`

    Vertical gap between windows in pixels.

## Default
```yaml
inner_gaps:
  x: 10
  y: 10
```
