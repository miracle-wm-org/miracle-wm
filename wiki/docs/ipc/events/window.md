# window (0x80000003)
Sent whenever an event involving a window occurs such as opening, focusing, or closing.

## Payload
```json
{
    "change": "string",
    "container": "object"
}
```

The `change` string is one of the following:

| TYPE            | DESCRIPTION                                              |
|-----------------|----------------------------------------------------------|
| new             | The window was created                                   |
| close           | The window was closed                                    |
| focus           | The window was focused                                   |
| fullscreen_mode | The window's fullscreen mode has changed                 |
| move            | The window has been reparented in the tree               |
| mark            | A mark has been added or removed from the window         |

The `container` is an object representing the container for that window. You may
see an example below.


## Example
```json
{
    "change": "new",
    "container": {
            "id": 12,
            "name": null,
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
            "layout": "none",
            "percent": 0.0,
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
                "width": 1124,
                "height": 422
            },
            "window": 4194313,
            "urgent": false,
            "floating_nodes": [
            ],
            "type": "con",
            "pid": 19787,
            "app_id": null,
            "window_properties": {
                "class": "URxvt",
                "instance": "urxvt",
                "transient_for": null
            },
            "nodes": [
            ]
    }
    }
```
