# GET_TREE (4)
Retrieves the container tree.

## Payload
Empty

## Reply
The reply is a tree of objects, where each object in the tree
has some associated metadata as well as a number of "nodes"
branching out underneath it, culminating in "leaf" nodes that
represent individual windows.

Each node is either an output, a workspace, a container that
contains other containers, or a window.

```json
{
    // A unique identifier for the node.
    "id": integer,      

    // Name of the node.
    "name": string,     

    // The number of the workspace.
    //
    // For workspaces nodes only.
    "num": integer, 

    // Type of the node.
    "type": "root" | "output" | "workspace" | "con" | "floating_con",

    // Current layout scheme on the node.
    "layout": "output" | "splith" | "splitv" | "stacking" | "tabbed" | "none",

    // Whether or not the node is visible.
    "visible": boolean,

    // Whether or not the node is focused.
    "focused": boolean,

    // Whether or not the node is "urgent".
    //
    // Urgent means that the node currently wants focus.
    // This is always 'false' as of now.
    "urgent": false,

    // The border style of the node.
    "border": "normal" | "none" | "pixel",

    // The size of the border if one exists.
    "current_border_width": integer,

    // An array of child nodes that are logically contained
    // within this node. Each object in this tree will have
    // the same shape as this JSON payload. 
    "nodes": object[],

    // The absolute display coordinates of the node.
    "rect": {
        "x": integer,
        "y": integer,
        "width": integer,
        "height": integer
    },

    // The rectangle of the window relative to its container.
    //
    // Only defined for nodes which contain a single window.
    "window_rect": {
        "x": integer,
        "y": integer,
        "width": integer,
        "height": integer
    },

    // The rectangle of the window's decorations relative to its container.
    // Decorations include both gaps and borders.
    //
    // Only defined for nodes which contain a single window.
    "deco_rect": {
        "x": integer,
        "y": integer,
        "width": integer,
        "height": integer
    },

    // The original geometry that the window specified before miracle
    // mapped it. This is used when switching between floating and tiling
    // mode.
    //
    // Only defined for nodes which contain a single window.
    "geometry": {
        "x": integer,
        "y": integer,
        "width": integer,
        "height": integer
    },

    // ID of the window contained in this node.
    //
    // This is the same as the "id" field.
    //
    // Only defined for nodes which contain a single window.
    "window": integer,

    // Whether or not the node is powered on.
    //
    // For output nodes only.
    "active": boolean,  

    // Whether or not the output is on.
    //
    // For output nodes only.
    "dpkms": boolean,

    // The scale of the output.
    //
    // For output nodes only.
    "scale": float,

    // Supplied but unused and always 'linear'.
    //
    // For outputs nodes only.
    "scale_filter": "linear",

    // Supplied but unused and always 'false'.
    //
    // For output nodes only.
    "adaptive_sync_status": false,

    // Make of the output.
    //
    // For output nodes only.
    "make": string,
 
    // Model of the output.
    //
    // For output nodes only.
    "model": string,

    // Serial number of the output.
    //
    // For output nodes only.
    "serial": string,

    // Transform of the output.
    //
    // For output nodes only.
    "transform": "normal" | "90" | "180" | "270",

    // Deprecated so always "none".
    //
    // For output nodes only.
    "orientation": "none",

    // List of available modes for this output.
    //
    // For output nodes only.
    "modes": {
        "width": integer,
        "height": integer,

        // Refresh rate in millihertz
        "refresh": integer
    }[],

    // Current mode for this output.
    //
    // For output nodes only.
    "current_mode": {
        "width": integer,
        "height": integer,

        // Refresh rate in millihertz
        "refresh": integer
    },

    // Whether or not this node is stuck to the screen.
    //
    // For container nodes only.
    "sticky": boolean,

    // The scratchpad state of the container.
    //
    // For container nodes only.
    "scratchpad_state": "none" | "fresh" | "changed" | "unknown",

    // The shell that this container is runnign in. Always
    // 'miracle-wm' for now.
    //
    // For container nodes only.
    "shell": "miracle-wm",

    // Unused for now.
    //
    // For container nodes only.
    "inhibit_idle": false,

    // Unused for now.
    //
    // For container nodes only.
    "idle_inhibitors": {
        "application": "none",
        "user": "visible"
    },

    // Whether or not the container is fullscreen on its output.
    //
    // For container nodes only.
    "fullscreen_mode": 1 | 0,

    // The percent of the parent node that this container takes.
    // The value is always between [0, 1].
    //
    // For container nodes only.
    "percent": float,

    // Arbitrary properties on a window. Unused for now and always empty.
    //
    // For container nodes only.
    "window_properties": object
}
```

## Example
The following is a very simple example with the only running application
being `kitty` in a hosted wayland session:

```json
{
  "id": 0,
  "name": "root",
  "nodes": [
    {
      "active": true,
      "adaptive_sync_status": false,
      "border": "none",
      "current_border_width": 0,
      "current_mode": {
        "height": 1024,
        "refresh": 60000.0,
        "width": 1280
      },
      "deco_rect": {
        "height": 0,
        "width": 0,
        "x": 0,
        "y": 0
      },
      "dpms": true,
      "focused": true,
      "geometry": {
        "height": 0,
        "width": 0,
        "x": 0,
        "y": 0
      },
      "id": 101330841730944,
      "layout": "output",
      "make": "Unknown",
      "model": "Unknown",
      "modes": [
        {
          "height": 1024,
          "refresh": 60000.0,
          "width": 1280
        }
      ],
      "name": "unknown-1",
      "nodes": [
        {
          "border": "none",
          "current_border_width": 0,
          "deco_rect": {
            "height": 0,
            "width": 0,
            "x": 0,
            "y": 0
          },
          "floating_nodes": [],
          "focused": true,
          "geometry": {
            "height": 0,
            "width": 0,
            "x": 0,
            "y": 0
          },
          "id": 101330847378496,
          "layout": "splith",
          "name": "1",
          "nodes": [
            {
              "app_id": "kitty",
              "border": "normal",
              "current_border_width": 2,
              "deco_rect": {
                "height": 1014,
                "width": 1270,
                "x": 0,
                "y": 0
              },
              "floating_nodes": [],
              "focus": [],
              "focused": true,
              "fullscreen_mode": 0,
              "geometry": {
                "height": 1014,
                "width": 1270,
                "x": 0,
                "y": 0
              },
              "id": 133916745441920,
              "idle_inhibitors": {
                "application": "none",
                "user": "visible"
              },
              "inhibit_idle": false,
              "layout": "none",
              "name": "",
              "nodes": [],
              "orientation": "none",
              "percent": 1.0,
              "pid": 15286,
              "rect": {
                "height": 1014,
                "width": 1270,
                "x": 5,
                "y": 5
              },
              "scratchpad_state": "none",
              "shell": "miracle-wm",
              "sticky": false,
              "type": "con",
              "urgent": false,
              "visible": true,
              "window": 133916745441920,
              "window_properties": {},
              "window_rect": {
                "height": 1010,
                "width": 1266,
                "x": 7,
                "y": 7
              }
            }
          ],
          "num": 1,
          "orientation": "none",
          "output": "unknown-1",
          "rect": {
            "height": 1014,
            "width": 1270,
            "x": 5,
            "y": 5
          },
          "type": "workspace",
          "urgent": false,
          "visible": true,
          "window": null,
          "window_rect": {
            "height": 0,
            "width": 0,
            "x": 0,
            "y": 0
          }
        }
      ],
      "orientation": "none",
      "rect": {
        "height": 1024,
        "width": 1280,
        "x": 0,
        "y": 0
      },
      "scale": 1.0,
      "scale_filter": "linear",
      "serial": "Unknown",
      "transform": "normal",
      "type": "output",
      "urgent": false,
      "visible": true,
      "window_rect": {
        "height": 0,
        "width": 0,
        "x": 0,
        "y": 0
      }
    }
  ],
  "rect": {
    "height": 1024,
    "width": 1280,
    "x": 0,
    "y": 0
  },
  "type": "root"
}
```
