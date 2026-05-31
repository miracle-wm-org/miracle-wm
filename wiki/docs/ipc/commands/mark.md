# mark
A `mark` is a convenient way to tag a container such that you can easily reference
it later. A window is marked using the `mark` command and unmarked using the `unmark`
command. Specifying `unmark` with no identifier removes all marks from the specified
containers. Marks do not need to be unique, meaning that multiple containers can share a mark.

When marking, you can specify a number of options:

1. the `--toggle` option will remove the mark if it already exists for that container, otherwise add it
2. the `--add` option will add the mark in addition to existing marks
3. the `--replace` option will remove all existing marks and replace them with the specified mark

## Syntax
```
mark [--add|--replace] [--toggle] <identifier>
[con_mark="identifier"] focus
unmark <identifier>
```

## Example
```sh
miraclemsg mark hi # Marks the focused container as 'hi'
unmark             # Removes all marks from the focused container
```


