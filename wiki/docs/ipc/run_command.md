# RUN_COMMAND (0)

## Payload
A string representing a command(s) to run.

## Reply
A list of objects each corresponding to the success of an individual
command:

```json
[
    {
        "success": boolean,
        "parse_error": boolean?, // True if the command failed at parse time, most likely due to an unknown command
        "error": string? // Human-readable error message
    }
]
```

`parse_error` and `error` will be omitted when `success` is true.

### Example
For example, if a user sends a `RUN_COMMAND` with the following payload:

```
resize grow width 10; meow 5
```

then they should see a reply that looks like this:

```json
[
    {
        "success": true
    },
    {
        "success": false,
        "parse_error": true,
        "error": "Unsupported command type: meow"
    }
]

```