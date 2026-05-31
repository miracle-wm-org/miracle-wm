# rename
The `rename` command lets you rename a workspace. If you try to
rename a workspace to a name or number that is already taken, the rename
will be ignored.

## Syntax
```sh
rename workspace <old_name> to <new_name> # Renames the workspace with <old_name> to <new_name>. Names are provided by the string: "number: name"
rename workspace to <new_name>            # Renames the selected workspace
```

### Example
```sh
miraclemsg "rename workspace 1 to \"2: hi\"" # Rename workspace to to have number=2 and name=hi
miraclemsg "rename workspace to 3"           # Rename the selected workspace to have number=2
```
