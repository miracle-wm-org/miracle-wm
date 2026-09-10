# config_errors (0x80000016)
Sent when the compositor (re)loads its configuration and collects parse errors. A client
subscribes to this event with the `"config_errors"` type (See [SUBSCRIBE](../subscribe.md)).

Upon subscribing, the client immediately receives a `config_errors` event containing all
errors from the most recent configuration load (an empty array if there were none).
Thereafter, the event is sent again each time the configuration is reloaded.

This event powers the bundled error reporter client. See
[WM Clients](../../configuration/wm_clients.md) for how to configure or disable it.

## Payload
A JSON array of error objects:
```json
[
    {
        "filename": string, // Path to the configuration file the error came from
        "line": number,     // Line number of the error (1-based)
        "column": number,   // Column number of the error (1-based)
        "level": string,    // "warning" or "error"
        "message": string   // Human-readable description of the error
    }
]
```

### Example
```json
[
    {
        "filename": "/home/user/.config/miracle-wm/config.yaml",
        "line": 12,
        "column": 3,
        "level": "error",
        "message": "Cannot find requested terminal program: notaterminal"
    }
]
```
