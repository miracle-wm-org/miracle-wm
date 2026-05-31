# shutdown (0x80000008)
Sent right before the miracle shuts down.

## Payload
```json
{
    "change": "restart" | "exit"
}
```

## Example
The following will be sent during a normal shutdown scenario:

```json
{
    "change": "exit'
}
```
