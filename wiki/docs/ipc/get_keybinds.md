# GET_KEYBINDS (202)
Retrieves the effective key binding set of the running configuration: the compositor's
built-in defaults, any `default_action_overrides`, and any `custom_actions`. This is a
miracle-specific message, intended for bars, cheat-sheet overlays and configuration
tooling that would otherwise have to re-parse `~/.config/miracle-wm.yaml` and duplicate
the built-in default table.

Keys are reported as XKB keysyms: both the numeric value and the name produced by
`xkb_keysym_get_name()`, which is the same spelling that `xkb_keysym_from_name()` — and
the `key` field of the configuration file — accepts. Modifiers are reported by name,
using the same vocabulary the configuration file accepts (`"shift"`, `"meta"`, ...).

Note that the field names here (`xkb_keysym`, `xkb_keysym_name`) deliberately differ from
the `input_code`/`symbol` fields of the [`binding` event](events/binding.md). That event is
wire-compatible with sway and must keep sway's naming; this message is miracle-specific
and names its fields after what they actually contain.

## Payload
Empty

## Reply
```json
{
    // The resolved primary modifier (the configured `action_key`). Reported once at
    // the top level rather than repeated on every entry.
    "primary_modifier": {
        "modifiers": ["meta"],
        "modifier_mask": 4096
    },

    // Every binding, in the order that the compositor attempts to match them:
    // custom actions first, then built-in overrides, then the built-in defaults.
    "keybinds": [
        {
            // Where the binding came from:
            //   "built_in_default"  - an entry of the compositor's built-in table
            //   "built_in_override" - a `default_action_overrides` entry
            //   "custom"            - a `custom_actions` entry
            "source": "custom",

            // The built-in action name ("terminal", "move_left", ...).
            // null when `source` is "custom".
            "action": null,

            // The shell command to run. null for both built-in sources.
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

## Notes on the reported set
These are all real runtime behaviours of the compositor, reported faithfully rather than
smoothed over:

- **Overrides are additive.** After trying the overrides, the compositor still tries the
  *whole* built-in default table, so the original default key keeps working unless the
  override happens to reuse the same key and modifiers. An override for `terminal`
  therefore produces **two** entries with `"action": "terminal"`: one tagged
  `built_in_override` and one tagged `built_in_default`.
- **The default table itself contains duplicate key/modifier pairs.** `resize_up` and
  `select_up` (and their down/left/right counterparts) are both `primary`+arrow. The
  resize bindings only accept the key while a resize mode is active. Entries are listed
  in the order they are tried, and the first handler to accept wins.
- **The workspace-move defaults encode the shifted keysym.** `move_to_workspace_1` is
  reported as `exclam` with no `shift` modifier, because the matcher ignores a `shift`
  bit that the binding did not ask for.
