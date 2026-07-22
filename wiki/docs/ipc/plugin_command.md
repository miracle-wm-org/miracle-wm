# PLUGIN_COMMAND (201)
Routes an arbitrary command to a [plugin](../plugins.md) that has registered a namespace.
This is a miracle-specific message: it lets an IPC client talk directly to a loaded plugin,
passing an arbitrary JSON payload and receiving an arbitrary JSON response back.

The command is delivered only to the plugin that owns the requested namespace. A namespace is
owned by at most one plugin (see [Plugins → IPC integration](../plugins.md#ipc-integration)).

## Payload
```json
{
    "plugin": string, // The namespace of the plugin to route the command to
    "payload": any    // Arbitrary JSON handed verbatim to the plugin
}
```

### Example
```json
{
    "plugin": "my-plugin",
    "payload": { "action": "toggle", "value": 42 }
}
```

## Reply
```json
{
    "success": boolean,
    "response": any,   // Present when success is true: the plugin's JSON response
    "error": string    // Present when success is false
}
```

`success` is `false` when no plugin owns the requested namespace, when the owning plugin does
not implement a command handler, or when the request/response is not valid JSON.

### Example
```json
{
    "success": true,
    "response": { "toggled": true }
}
```

## Notes
- The paired event type is [plugin](events/plugin.md), which a plugin uses to push data to
  subscribed clients on the same namespace.
