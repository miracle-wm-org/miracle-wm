# GET_WORKSPACES (1)
Retrieves the list of workspaces.

## Payload
Empty

## Reply
The reply is an array of objects corresponding to the following schema:

```json
[
    {
        "num": integer,     // The workspace number, or -1 for workspaces without a number
        "name": string,     // The name of the workspace
        "visible": boolean, // True if the workspace is currently visible on an output, otherwise false
        "focused": boolean, // True if the workspace is visible and its output is currently selected
        "urgent": boolean,  // Legacy, awlways false
        "output" string,    // Name of the output that the workspace is on
        "rect": {           // The rectangle defining this workspace
            "x" integer,
            "y": integer,
            "width": integer,
            "height": integer
        }
    }
]
```

### Example
```json
[
    {
        "num": 1,
        "name": "1",
        "visible": true,
        "focused": true,
        "output": "eDP-1",
        "rect": {
            "x": 0,
            "y": 23,
            "width": 1920,
            "height": 1057
        },
    }
]
```