# mode (0x80000002)
Send when the current mode of the compositor changes.

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
