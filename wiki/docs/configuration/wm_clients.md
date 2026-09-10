# WM Clients
Configures the helper clients that ship with `miracle-wm`. These are clients that
the compositor launches on your behalf to provide extra functionality. Today these are the
**error reporter**, which displays configuration errors fullscreen so that you can see them
without reading the logs, and the **debug overlay**, which draws live debugging information
on top of every window. More clients (decorations, top bars, etc.) will be configured here
in the future.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

wm_clients:
  error_reporter: "default"
  debug_overlay: "default"
```

## Schema

```yaml
wm_clients:
  error_reporter?: "default" | "disabled" | <string>
  debug_overlay?: "default" | "disabled" | <string>
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

### `debug_overlay`

:   **type:** String  
    **Default:** `"default"`

    Selects the client used to draw the [debug overlay](../ipc/commands/debug.md). The
    overlay is toggled at runtime with `miraclemsg debug` and draws window geometry, clip
    areas, the window under the cursor, the cursor position, and a list of hidden windows on
    top of everything else.

    Accepts one of:

    - `"default"` — use the bundled `miracle-wm-debug-overlay` client.
    - `"disabled"` — never launch a debug overlay. `miraclemsg debug` becomes a no-op.
    - any other string — treated as the path or name of an executable to launch instead of
      the bundled overlay.

## Default
```yaml
wm_clients:
  error_reporter: "default"
  debug_overlay: "default"
```
