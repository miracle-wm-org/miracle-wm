# GET_BINDING_MODES (8)
Retrieves a list of valid binding modes. The "default" binding mode is always in the list.

## Payload
Empty

## Reply
```json
[
    string
]
```

### Example
```json
[
    "default",
    "resize",
    "selecting",
    "dragging",
    "moving"
]
```