# SUBSCRIBE (2)
Subscribe this IPC connection to the event types specified in the message payload.

## Payload
An array of events names. Events are specified according to the following
table:

| Value | Name | Description |
| ----- | ---- | ----------- |
| 0x80000000 | workspace | sent whenever an event involving a workspace occurs such as creation, focus gain, or removal |
| 0x80000001 | output | sent whenever an event involving an output occurs |
| 0x80000002 | mode | sent whenever the mode changes |
| 0x80000003 | window | sent whenever an event involving a window occurs such as creation, focus gain, or removal |
| 0x80000006 | shutdown | sent when the compositor is about to shutdown |
| 0x80000007 | tick | sent when an ipc client sends a [SEND_TICK](send_tick.md) message |
| 0x80000015 | input | sent when something related to input changes |

### Example

```json
[ "workspace", "mode" ]
```

## Reply
```json
{
    "success": boolean,
    "error" string?
}
```

### Example
```json
{
    "success": true
}
```