# binding (0x80000005)
Sent whenever a binding is triggered in resposne to keyboard or
mouse input.

## Payload
```json
{
    "change": "run", // Always 'run'
    "binding": {
        "command": "string",              // Command that was run
        "event_state_mask": "string[]",   // Moifiers being held
        "input_code": "integer",          // Xkb keysym
        "symbol": "string",               // Stringified keysym 
        "input_type": "keyboard | mouse"  // Input type
    }
}
```


## Example
```json
{
    "change": "run",
    "binding": {
            "command": "workspace 2",
            "event_state_mask": [
                "meta",
            ],
            "input_code": 0,
            "symbol": "2",
            "input_type": "keyboard"
    }
}
```
