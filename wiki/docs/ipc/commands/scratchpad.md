# scratchpad
The scratchpad is an off-screen workspace to which users can move containers
and then show them later. When the container is shown, it appears in the center
of the current workspace. This is useful if you want to repeatedly show and
hide a window (e.g. a terminal doing some sort of compilation).

To remove the container from the scratchpad, simply issue a `floating toggle`
command to return it to the grid.

## Syntax
```sh
# Moves the current container to the scratchpad. Note that this will effectively
# "float" the container out of its tile if it is in a tile.
move scratchpad

# Toggle the visibility of the container on the scratchpad. The container will
# appear in the center of the current workspace. Calling 'scratchpad show'
# on an already-shown scratchpad window will hide it.
scratchpad show
```

## Example
```sh
miraclemsg move scratchpad              # Move the current window to the scratchpad
miraclemsg scratchpad show              # Show the first window on the scratchpad

# Show the window with the pid of 1234 on the scratchpad and hide the previously
# shown scratchpad window.
miraclemsg [pid="1234"] scratchpad show
```
