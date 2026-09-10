# mode (0x80000002)
Send when the current mode of the compositor changes.

The `change` field is one of the modes listed by
[GET_BINDING_MODES](../get_binding_modes.md): `default`, `resize`, `dragging`,
`moving`, or `overview` (the window overview/spread is on screen).

## Payload
```json
{
    "change": "string",
    "pango_markup": true  // Always true
}
```

## Eample
```json
{
    "change": "resize",
    "pango_markup": true
}
```
