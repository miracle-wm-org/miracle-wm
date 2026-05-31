# workspace (0x80000000)
Sent whenever an event involving a workspace occurs such as creation,
focusing, removal.

## Payload
```json
{
    "change": "string",
    "current": "object", // An object representing the workspace or null for 'reload' changes
    "old": "object" // For a 'focus' change, an object representing the workspace being switched from. Otherwise, null.
}
```

The `change` string is one of the following:


| TYPE   | DESCRIPTION                                                                                   |
|--------|-----------------------------------------------------------------------------------------------|
| init   | The workspace was created                                                                     |
| empty  | The workspace is empty and is being destroyed since it is not visible                         |
| focus  | The workspace was focused. See the old property for the previous focus                        |
| move   | The workspace was moved to a different output                                                 |
| rename | The workspace was renamed                                                                     |
| reload | The configuration file has been reloaded                                                      |

## Example
```json

{
    "change": "init",
    "old": null,
    "current": {
            "id": 10,
            "name": "2",
            "rect": {
                "x": 0,
                "y": 0,
                "width": 0,
                "height": 0
            },
            "focused": false,
            "focus": [
            ],
            "border": "none",
            "current_border_width": 0,
            "layout": "splith",
            "percent": null,
            "window_rect": {
                "x": 0,
                "y": 0,
                "width": 0,
                "height": 0
            },
            "deco_rect": {
                "x": 0,
                "y": 0,
                "width": 0,
                "height": 0
            },
            "geometry": {
                "x": 0,
                "y": 0,
                "width": 0,
                "height": 0
            },
            "window": null,
            "urgent": false,
            "floating_nodes": [
            ],
            "num": 2,
            "output": "eDP-1",
            "type": "workspace",
            "representation": null,
            "nodes": [
            ]
    }
}
```
