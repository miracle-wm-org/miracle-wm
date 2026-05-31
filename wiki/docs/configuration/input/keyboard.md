# Keyboard Configuration
Configure the keyboard input device.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

# Use the english + dvorak keyboard layout.
keyboard:
    keymap:
        language: en
        variant: dvorak
        options: []
```

## Schema

```yaml
keyboard:
  keymap:
    language: <string>
    variant?: <string>
    options?: <string[]>
```

## Properties

### `keymap`

:   <small>required</small> **type:** Object

    The keyboard layout configuration.

#### `language`

:   <small>required</small> **type:** String

    The keyboard layout language. You may list the available languages using:
    ```sh
    localectl list-x11-keymap-layouts
    ```

#### `variant`

:   **type:** String

    The variant of the language. You may list the available variants using:
    ```sh
    localectl list-x11-keymap-variants
    ```

#### `options`

:   **type:** List of strings

    Options on the language variant. These are only applied if a variant is supplied. You may list the available options using:
    ```sh
    localectl list-x11-keymap-options
    ```

## Default
```yaml
keyboard:
  keymap:
    language: us
    variant: null
    options: []
```
