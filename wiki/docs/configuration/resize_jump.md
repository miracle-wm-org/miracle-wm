# Resize Jump
Defines the number of pixels that a window will be resized by for each resize request.
This value affects any direction (up, down, left, or right).

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

resize_jump: 25 # Each resize will now add or remove 25px from the current window size
```

## Schema

```yaml
resize_jump: <int>
```

## Properties

### `resize_jump`

:   <small>required</small> **type:** Integer

    The number of pixels to resize a window by for each resize request in any direction (up, down, left, or right).

## Default
```yaml
resize_jump: 50
```
