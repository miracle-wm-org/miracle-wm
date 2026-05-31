# GET_VERSION (7)
Retrieves information about the version of miracle that is running.

## Payload
Empty

## Reply
```json
{
    "major": integer, // The major version
    "minor": integer, // The minor version
    "patch": integer, // The patch version
    "human_readable": string, // A readable version string
    "loaded_config_file_name": string, // Full path to the configuration file
}
```

### Example
```json
{
    "major": 0,
    "minor": 5,
    "patch": 2,
    "human_readable": "0.5.2",
    "loaded_config_file_name": "/home/mattkae/.config/miracle-wm/config.yaml"
}
```