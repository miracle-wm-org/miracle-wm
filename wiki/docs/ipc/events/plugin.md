# plugin (0x80000017)
Sent when a loaded [plugin](../../plugins.md) publishes an event on its namespace.

Unlike the other events, a client subscribes to `plugin` events **per namespace**: it provides
the plugin namespace it is interested in, and only events published on that namespace are
delivered. See [SUBSCRIBE](../subscribe.md) for the subscription payload.

A namespace is owned by at most one plugin. The paired message type is
[PLUGIN_COMMAND](../plugin_command.md), which routes a command from a client to the plugin.

## Payload
```json
{
    "plugin": string, // The namespace of the plugin that published the event
    "payload": any    // The arbitrary JSON payload published by the plugin
}
```

### Example
```json
{
    "plugin": "my-plugin",
    "payload": { "state": "connected", "battery": 87 }
}
```
