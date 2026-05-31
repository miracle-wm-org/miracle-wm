# SYNC (11)
Like sway, this this command will just return a failure object since it does not
make sense to implement in sway due to the X11 nature of the command. If you 
are curious about what this IPC command  does  in  i3, refer to the i3 documentation.

## Payload
None

## Reply
```json
{
    "name": "default"
}
```