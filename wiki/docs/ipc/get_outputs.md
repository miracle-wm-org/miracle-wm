# GET_OUTPUTS (3)
Retrieves the list of outputs.

## Payload
Empty

## Reply
The reply is an array of objects corresponding to the following schema:

```json
[
    {
        "name": string, // Name of the output
        "make": string, // Make of the output
        "model": string, // Model of the output
        "serial": string, // The outputs serial number as a hexadecimal string
        "active": boolean, // Whether or not this output is used
        "dpms": boolean, // Deprecated. True if the output is on, otherwise false
        "power": boolean, // True if the output is on, otherwise false
        "primary": boolean, // True if output is the primary output, otherwise false
        "scale": float, // Scale of the output, or -1 if not used
        "subpixel_hinting": rgb | bgr | vrgb | vbgr | none, // The current subpixel hinting in use on the output
        "transform": normal | 90 | 180 | 270 | flipped-90 | flipped-180 | flipped-270, // The transform of the output
        "current_workspace": string, // The name of the current workspace on the output, or null for disabled outputs
        "modes": {  // An array of the supported modes on this output
            "width": integer,
            "height": integer,
            "refresh": double
        }[],
        "current_mode": { // The current mode of the output
            "width": integer,
            "height": integer,
            "refresh": double
        },
        "rect": { // The bounds of the output
            "x": integer,
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
        "name": "HDMI-A-2",
        "make": "Unknown",
        "model": "NS-19E310A13",
        "serial": "0x00000001",
        "active": true,
        "primary": false,
        "scale": 1.0,
        "subpixel_hinting": "rgb",
        "transform": "normal",
        "current_workspace": "1",
        "modes": [
            {
                "width": 640,
                "height": 480,
                "refresh": 59940
            },
            {
                "width": 800,
                "height": 600,
                "refresh": 60317
            },
            {
                "width": 1024,
                "height": 768,
                "refresh": 60004
            },
            {
                "width": 1920,
                "height": 1080,
                "refresh": 60000
            }
        ],
        "current_mode": {
            "width": 1920,
            "height": 1080,
            "refresh": 60000
        }
    }
]
```