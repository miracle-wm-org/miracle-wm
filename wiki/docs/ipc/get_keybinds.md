# GET_KEYBINDS (202)
Retrieves the effective key binding set of the running configuration. This is a
miracle-specific message, intended for bars, cheat-sheet overlays and configuration
tooling.

Keys are reported as XKB keysyms. Modifiers are reported by name,
using the same vocabulary the configuration file accepts (`"shift"`, `"meta"`, ...).

## Payload
Empty

## Reply
```json
{
    // The primary modifier (the configured `action_key`).
    "primary_modifier": {
        "modifiers": ["meta"],
        "modifier_mask": 4096
    },

    // Every binding, in the order that the compositor attempts to match them.
    "keybinds": [
        {
            // The built-in action to run ("terminal", "move_left", ...), if any.
            "action": null,

            // The shell command to run, if any.
            "command": "echo Hi",

            // "down", "up" or "repeat".
            "keyboard_action": "down",

            // The modifiers the user physically holds, with the primary modifier
            // already resolved. Never contains "primary".
            "modifiers": ["meta"],

            // The same resolved set, as a MirInputEventModifier bitfield.
            "modifier_mask": 4096,

            // The modifiers as written in the configuration. May contain "primary".
            "configured_modifiers": ["primary"],

            // The XKB keysym, numerically and by name.
            "xkb_keysym": 120,
            "xkb_keysym_name": "x"
        }
        // ...
    ]
}
```
