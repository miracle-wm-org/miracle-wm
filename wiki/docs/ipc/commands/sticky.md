# sticky
Sticky windows to "stick" to the glass meaning that they are always on top and
that they follow you when switch workspaces. This is useful for when you want
to watch something while doing other work, video conferences while your switching
workspace, and many other things.

Being "sticky" only applies to floating windows.

## Syntax
```sh
sticky enable|disable|toggle
```

## Example
```sh
miraclemsg sticky enable   # Make the focused window sticky
miraclemsg sticky disable  # Make the focused window not sticky
miraclemsg sticky toggle   # Toggle 'sticky' on the focused window (sticky=true in this case)
```
