# Hover Click
Hover Click is an accessibility feature that enables users to click by simply
hovering their cursor over a position for a period of time. This is useful for
users who find it hard to move the pointer and click at the same time.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

hover_click:
    enabled: true
    hover_duration: 5000
    cancel_displacement_threshold: 50
    reclick_displacement_threshold: 5
```

## Schema

```yaml
hover_click:
  enabled?: <boolean>
  hover_duration?: <uint>
  cancel_displacement_threshold?: <int>
  reclick_displacement_threshold?: <int>
```

## Properties

### `enabled`

:   **type:** Boolean  
    **Default:** `false`

    Whether the hover click feature is enabled.

### `hover_duration`

:   **type:** Unsigned Integer (milliseconds)  
    **Default:** `1000`

    The duration in milliseconds before a click is issued.

### `cancel_displacement_threshold`

:   **type:** Integer (pixels)  
    **Default:** `10`

    The pointer displacement in pixels required to cancel a pending click. This is useful for users who struggle to hold the pointer in one position without shaking it.

### `reclick_displacement_threshold`

:   **type:** Integer (pixels)  
    **Default:** `5`

    The pointer displacement in pixels that the pointer needs to move after a click in order for a new click to be issued.

## Default
```yaml
hover_click:
  enabled: false
  hover_duration: 1000
  cancel_displacement_threshold: 10
  reclick_displacement_threshold: 5
```
