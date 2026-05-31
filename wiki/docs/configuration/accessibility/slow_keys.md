# Slow Keys
The slow keys accessibility feature allows users to insert a delay between the
time that a key is pressed and the time that it is propagated to clients.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

slow_keys:
    enabled: true
    hold_delay: 5000  # 5 seconds, which is an outrageously large delay
```

## Schema

```yaml
slow_keys:
  enabled?: <boolean>
  hold_delay?: <uint>
```

## Properties

### `enabled`

:   **type:** Boolean  
    **Default:** `false`

    Whether or not slow keys is enabled.

### `hold_delay`

:   **type:** Unsigned Integer (milliseconds)  
    **Default:** `200`

    The time in milliseconds between key down and key propagation.

## Default
```yaml
slow_keys:
  enabled: false
  hold_delay: 200
```
