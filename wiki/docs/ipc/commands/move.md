# move
The `move` command provides a way to change the positions of containers and
workspaces.

## syntax
```sh
# Moves the container into the given direction.
# The optional pixel argument specifies how far the
# container should be moved if it is floating and
# defaults to 10 pixels. The optional ppt argument
# means "percentage points", and if specified it indicates
# how many points the container should be moved if it is
# floating rather than by a pixel value.
move <left|right|down|up> [<amount> [px|ppt]]

# Moves the container to the specified pos_x and pos_y
# coordinates on the screen.
move position <pos_x> [px|ppt] <pos_y> [px|ppt]

# Moves the container to the center of the screen.
# If 'absolute' is used, it is moved to the center of
# all outputs.
move [absolute] position center

# Moves the container to the current position of the
# mouse cursor. Only affects floating containers.
move position mouse

# Moves the container/window to the mark provided by <mark>,
# if the mark exists.
move window|container to mark <mark>

# Move the container to a workspace by name.
move [--no-auto-back-and-forth] [window|container] [to] workspace <name>

# Move the container to a workspace by number and optionally name.
move [--no-auto-back-and-forth] [window|container] [to] workspace number <name>

# Move the container to a workspace by direction.
move [window|container] [to] workspace prev|next|current

# Moves the active container to a different output.
move container to output left|right|down|up|current|primary|nonprimary|next|<output1> [output2]…

# Moves the active workspace to a different output.
# If the workspace is already on that output, then this is a noop.
# Multiple outputs may be specified by name, (e.g. move workspace
# to output VGA-1 HDMI-A-1), which instructs miracle to cycle the
# workspace through the list of outputs.
move workspace to output left|right|down|up|current|primary|nonprimary|next|<output1> [output2] ...
```

## Example
```sh
move left 20                    # Moves the focused container left by 20px, only if it is already floating
move position 10 ppt 10 ppt     # Move the focused container to the 10% position of the current output
move position center            # Move the focused container to the center of the current output
move container to mark meow     # Moves the focused container to mark "meow"
move container to workspace 1   # Moves the focused container to workspace 1
move workspace to output right  # Moves the current workspace to the output directly to the right of the active output
```

## Links
- [id documentation for "move ..."](https://i3wm.org/docs/userguide.html#move_direction)
- [i3 documentation for "move container/window to mark..."](https://i3wm.org/docs/userguide.html#move_to_mark)
- [i3 documentation for "move workspace"](https://i3wm.org/docs/userguide.html#_moving_workspaces_to_a_different_screen)
