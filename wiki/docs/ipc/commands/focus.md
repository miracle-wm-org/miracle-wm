# focus
This command provides a way to change the current focus for outputs, workspaces,
and containers.

## Syntax
```sh
# Sets focus to the container that matches the specific <criteria>
<criteria> focus

# Sets focus to the workspace of the container that matches the specific
# <criteria>
<criteria> focus workspace

# Sets the focus to the container in a particular direction if it exists.
focus left|right|down|up

# Sets focus to the parent of the current container, the first child of the
# current container, the first floating container, the first tiling container,
# or either a floating or a tiling container depending on the status of the
# currently selected container.
focus parent|child|floating|tiling|mode_toggle

# Sets focus to an adjacent container. If [sibling] is specified, the command will
# focus the exact sibling container, including non-leaf containers like split containers.
# Otherwise, it is very similar to focus left|right|down|up.
focus next|prev [sibling]

# Sets focus to an output by name, by direction, or by status (e.g. primary or nonprimary).
# If multiple names are specified, then focus will be cycled through the list.
focus output left|right|down|up|current|primary|nonprimary|next|<output1> [output2]…
```

## Example
```sh
miraclemsg [pid="1234"] focus           # Focus a container with pid = 1234
miraclemsg focus right                  # Focus the next container to the right
miraclemsg focus output VGA-1 VGA-2     # Focus the next output in the list
```

## Links
- [i3 documentation](https://i3wm.org/docs/userguide.html#_focusing_moving_containers)
