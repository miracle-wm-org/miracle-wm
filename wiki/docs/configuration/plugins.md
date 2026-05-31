# Plugins
Specify plugins that will be executed by the system.

Plugins will be executed by the order in which they appear in the list. For example,
if one plugin has a method that handles `place_new_window`, then the plugin that
follows it will *not* get to run its `place_new_window` method.

## Example

```yaml
# Use a plugin animation for window open
plugins:
  - path: /path/to/plug1.wasm
    data1: xyz
    data2: abc
  - path: /path/to/plug2.wasm
    data3: [1, 2, 3]
```

## Schema
A list of plugins with a path and an arbitrary number of keyed data that the plugin
is able to use in order to modify its behavior.

```yaml
plugins:
  - path: <string path>
    [key1]: <arbitary YAML>
    [key2]: <arbitary YAML>
    ...
    [keyn]: <arbitary YAML>
```

## Properties

### path
:   <small>required</small> **type:** String

    The path to the WebAssembly module that should be loaded as a plugin.

### [key1...n]
:   **type:** YAML

    Users may specify data that they would like to be supplied to the plugin at runtime.
    The plugin will have access to this arbitrary data so that their behavior
    is configurable by the user.

    See your specific plugin docs for the valid configuration details.

