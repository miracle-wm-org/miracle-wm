# Terminal
Select which terminal will be opened by the `terminal` keybind (See [Default Keybinds](default_keybinds.md)).

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

terminal: konsole # Set konsole as the default terminal
```

## Schema

```yaml
terminal: <string>
```

## Properties

### `terminal`

:   <small>required</small> **type:** String

    The shell command used to launch the default terminal application.

## Default
```yaml
terminal: miracle-mw-sensible-terminal
```

