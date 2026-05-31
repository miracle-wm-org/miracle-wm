# tick (0x80000007)
Sent when an ipc client sends a [SEND_TICK](../send_tick.md) message.

## Payload
```json
{
    "first": boolean, // Whether this event was triggered by subscribing to the tick events
    "payload": string, // The payload that was provided to a SEND_TICK message, if any. Otherwise an empty string.
}
```

### Example
```json
{
    "first": true,
    "payload": ""
}
```