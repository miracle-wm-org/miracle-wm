# SENT_TICK (10)
Issues a [TICK event](./events/tick.md) to all clients that are subscribed to tick events. This may be used
to ensure that all events prior to the tick are received.

## Payload
Users may provide an optional JSON payload. This payload will be provided back to the listeners
of the tick event.

## Reply
```json
{
    "success": boolean
}
```

### Example
```json
{
    "success": true
}
```