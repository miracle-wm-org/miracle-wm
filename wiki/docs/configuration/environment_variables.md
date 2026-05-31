# Environment Variables
A list of environment variables that are set when the compositor starts.

## Example

```yaml
# ~/.config/miracle-wm/config.yaml

environment_variables:
  - key: mesa_glthread  # Setting mesa_glthread to false fixes some AMD issues
    value: false
```

## Schema

A list of environment variable definitions:

```yaml
environment_variables:
  - key: <string>
    value: <any>
```

## Properties

### `key`

:   <small>required</small> **type:** String

    The name of the environment variable.

### `value`

:   <small>required</small> **type:** Any

    The value to assign to the environment variable. Can be a string, number, or boolean.

## Default
```yaml
environment_variables: []
```
