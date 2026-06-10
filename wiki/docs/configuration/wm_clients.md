# WM Clients
Configures the helper clients that ship with `miracle-wm`. These are internal clients that
the compositor launches on your behalf to provide extra functionality. Today the only such
client is the **error reporter**, which displays configuration errors fullscreen so that you
can see them without reading the logs. More internal clients (decorations, top bars, etc.)
will be configured here in the future.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

wm_clients:
  error_reporter: "default"
```

## Schema

```yaml
wm_clients:
  error_reporter?: "default" | "disabled" | <string>
```

## Properties

### `error_reporter`

:   **type:** String  
    **Default:** `"default"`

    Selects the client used to display configuration errors. When a configuration error is
    detected (for example, after editing your config), the compositor launches this client
    and it shows the errors fullscreen with an "X" to dismiss them.

    Accepts one of:

    - `"default"` — use the bundled `miracle-wm-basic-error-reporter` client.
    - `"disabled"` — do not launch any error reporter. Errors are still written to the logs.
    - any other string — treated as the path or name of an executable to launch instead of
      the bundled reporter.

## Default
```yaml
wm_clients:
  error_reporter: "default"
```
