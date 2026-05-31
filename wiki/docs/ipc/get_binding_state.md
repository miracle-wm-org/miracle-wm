# GET_BINDING_STATE (12)
Request  the  current  binding state, e.g. the currently active binding mode name.
This state is guaranted to be in the list returned by [GET_BINDING_MODES](./get_binding_modes.md)

## Type Number
12

## Payload
None

## Reply
```json
{
    "name": string // Name of the mode
}
```

### Example
```json
{
    "name": "default"
}
```