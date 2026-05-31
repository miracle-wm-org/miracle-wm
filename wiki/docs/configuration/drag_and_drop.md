# Drag and Drop
"Drag and drop" refers to functionality that enables users to drag containers
within or between a tiling grid. If enabled, the user may hold down the modifier
keys and the primary mouse button to begin dragging. The user then moves their
pointer above other containers to drag the active container into that position.
If the user releases the pointer, the container will snap to the desired
location.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

drag_and_drop:
  enabled: true
  modifiers:
    - primary
    - alt
```

## Schema

```yaml
drag_and_drop:
  enabled: <boolean>
  modifiers: <Modifier[]>
```

## Properties

### `enabled`

:   <small>required</small> **type:** Boolean  
    **Default:** `true`

    When `false`, drag and drop is disabled entirely. When `true`, drag and drop is enabled.

### `modifiers`

:   <small>required</small> **type:** List of modifier keys  
    **Default:** `[primary, shift]`

    List of modifiers that must be held when left clicking to initiate drag and drop.
    
    For a list of valid modifiers, see [Action Key](action_key.md).

## Default
```yaml
drag_and_drop:
  enabled: true
  modifiers:
    - primary
    - shift
```
