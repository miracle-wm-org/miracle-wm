# Workspaces
Customize a particular workspace.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

# Make it so that workspace 1 always has the name of "main".
workspaces:
    - number: 1
      name: main
```

## Schema

A list of workspace configurations:

```yaml
workspaces:
  - number?: <int>
    name?: <string>
```

## Properties

### `number`

:   **type:** Integer

    The number of the workspace. Either `name`, `number`, or both must be provided.

### `name`

:   **type:** String

    The name of the workspace. Either `name`, `number`, or both must be provided.

## Default
```yaml
workspaces:
  - number: 0
  - number: 1
  - number: 2
  - number: 3
  - number: 4
  - number: 5
  - number: 6
  - number: 7
  - number: 8
  - number: 9
```
