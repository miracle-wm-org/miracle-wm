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
| 0x80000016 | config_errors | sent when the configuration is (re)loaded, carrying any parse errors (See [config_errors](events/config_errors.md)) |
| 0x80000017 | plugin | sent when a plugin publishes an event on a subscribed namespace (See [plugin](events/plugin.md)) |

### Example

```json
[ "workspace", "mode" ]
```

## Plugin namespace subscriptions
The [plugin](events/plugin.md) event is subscribed to **per namespace**. A plugin namespace is
passed as a plain string, exactly like the ordinary event names above, and only plugin events
published on that namespace are delivered to the connection. A namespace must not be one of the
preset event names in the table above; any string that is not a preset event name is treated as
a plugin namespace. Namespaces may be mixed freely with the ordinary event-name strings.

### Example
```json
[ "workspace", "my-plugin", "other-plugin" ]
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