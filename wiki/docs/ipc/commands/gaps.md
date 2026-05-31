# gaps
Introduced in v0.7.0, the `gaps` command allows users to change both
the inner and out caps either globally or per-workspace.

## Syntax
```sh
# Inner gaps: space between two adjacent windows (or split containers).
gaps inner current|all set|plus|minus <gap_size_in_px>
# Outer gaps: space along the screen edges.
gaps outer|horizontal|vertical|top|right|bottom|left current|all set|plus|minus <gap_size_in_px>
```

## Example
```sh
miraclemsg gaps outer all set 100 # Set outer gaps on all workspaces to 100px
miraclemsg gaps inner current set 10 # Set inner gaps on the current workspace to 10px
```

## Links
- [i3 documentation](https://i3wm.org/docs/userguide.html#changing_gaps)
