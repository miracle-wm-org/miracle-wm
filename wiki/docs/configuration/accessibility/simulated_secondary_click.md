# Simulated Secondary Click
Simulated secondary click enables the user to issue a right-click by holding
down the left mouse button for some duration of time.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

simulated_secondary_click:
    enabled: true
    hold_duration: 5000
    displacement_threshold: 50
```

## Schema

```yaml
simulated_secondary_click:
  enabled?: <boolean>
  hold_duration?: <uint>
  displacement_threshold?: <int>
```

## Properties

### `enabled`

:   **type:** Boolean  
    **Default:** `false`

    Whether simulated secondary click is enabled.

### `hold_duration`

:   **type:** Unsigned Integer (milliseconds)  
    **Default:** `1000`

    The duration in milliseconds that the left button must be held before a right-click is issued.

### `displacement_threshold`

:   **type:** Integer (pixels)  
    **Default:** `10`

    A displacement in pixels that the pointer is allowed to move by while holding the left mouse button before the right-click is cancelled.

## Default
```yaml
simulated_secondary_click:
  enabled: false
  hold_duration: 1000
  displacement_threshold: 10
```

