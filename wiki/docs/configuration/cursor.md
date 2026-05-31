# Cursor
Describes the cursor configuration.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

cursor:
    scale: 4.0 # Sets the cursor scale to 4 times its original size.
    focus_mode: click # Click a mouse button while hovering a window to focus
    theme: redglass # The theme of the cursor
```

## Schema

```yaml
cursor:
  scale?: <float>
  focus_mode?: <hover|click>
  theme?: <string>
```

## Properties

### `scale`

:   **type:** Float  
    **Default:** `1.0`

    The scale multiplier of the cursor. A value of `2.0` makes the cursor twice its original size.

### `focus_mode`

:   **type:** `hover` | `click`  
    **Default:** `hover`

    How to focus windows with the cursor:
    
    - `hover` — Focus windows by hovering over them with the cursor
    - `click` — Click a mouse button while hovering a window to focus it

### `theme`

:   **type:** String  
    **Default:** System default

    The name of the cursor theme to use (e.g. `redglass`, `Adwaita`, `DMZ-White`).

    !!! warning
        The cursor size is currently fixed at 24x24 regardless of the `scale` setting. This is a known limitation and will be addressed in a future release of Mir.

    To find available themes on your system, look in the following directories:

    - `~/.local/share/icons/`
    - `/usr/share/icons/`

    Any directory in those locations that contains a `cursors/` subdirectory is a valid cursor theme. You can also list them with:

    ```sh
    find /usr/share/icons ~/.local/share/icons -maxdepth 2 -name "cursors" -type d | sed 's|/cursors||'
    ```

    #### Propagating the theme to X clients

    miracle-wm sets the cursor theme for Wayland clients, but X clients (running via XWayland) read the theme from `~/.Xresources`. To keep them consistent, add the following to your `~/.Xresources`:

    ```
    Xcursor.theme: redglass
    Xcursor.size: 24
    ```

    Then reload it with:

    ```sh
    xrdb -merge ~/.Xresources
    ```


## Default
```yaml
cursor:
  scale: 1.0
  focus_mode: hover
```
