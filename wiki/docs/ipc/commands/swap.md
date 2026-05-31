# swap
Two containers can be swapped using the `swap` command. Each container will assume the position
and geometry of the container that it was swapped with.

The first container participating in the swapping can be selected via command criteria. The
second container can be selected using one of the following methods:

- **id**: the app ID of the window
- **mark**: a container with the specified mark

## Syntax
```sh
swap container with id|mark <arg>
```

## Example
```sh
miraclemsg mark swapee # Marks the focused container as 'swapee'
miraclemsg focus left  # Focus the container to the left of 'swapee'
miraclemsg swap container with mark swapee  # Swaps the focused container with the container marked 'swapee'
```

## Links
- [i3 documentation](https://i3wm.org/docs/userguide.html#swapping_containers)
