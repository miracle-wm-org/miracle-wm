# layout

Introduced in v0.4.0, the `layout` command allows users to to change the layout
of the currently selected container.

## Syntax
```sh
layout default|tabbed|stacking|splitv|splith
layout toggle [split|all]
layout toggle [split|tabbed|stacking|splitv|splith] [split|tabbed|stacking|splitv|splith] ...
```

## Example
```sh
miraclemsg "layout splitv"  # Set the layout to 'vertical'
miraclemsg "layout splith"  # Set the layout to 'horizontal'
miraclemsg "layout toggle split"  # Set the layout to 'vertical' if 'horizontal' and vice versa
miraclemsg "layout toggle tabbed splith splitv"  # Cycle through the layouts in the list
```

## Links
- [i3 documentation](https://i3wm.org/docs/userguide.html#manipulating_layout)
