# Startup Apps
A list of applications that will be started when the compositor starts.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

# Start waybar and swaybg on startup
startup_apps:
  - command: waybar
    restart_on_death: true
  - command: swaybg -i /path/to/my/image
    restart_on_death: true
    in_systemd_scope: true
```

## Schema

A list of startup application definitions:

```yaml
startup_apps:
  - command: <string>
    restart_on_death?: <boolean>
    in_systemd_scope?: <boolean>
```

## Properties

### `command`

:   <small>required</small> **type:** String

    A shell command to execute on startup.

### `restart_on_death`

:   **type:** Boolean  
    **Default:** `false`

    If `true`, the application will automatically restart if it crashes or exits.

### `in_systemd_scope`

:   **type:** Boolean  
    **Default:** `false`

    If `true`, the application will be started with `systemd-run --user --property Restart=on-failure <COMMAND>`.

## Default
```yaml
startup_apps: []
```
