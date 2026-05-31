# Workspace Back and Forth

When enabled, `workspace_back_and_forth` makes it so that navigating to the currently active workspace causes you to navigate to the last focused workspace. If this feature is disabled, nothing will happen when navigating to the current workspace.

For example, if workspace 1 is active and you switch to workspace 2, then try to switch to workspace 2 again, workspace 1 will become active.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

workspace_back_and_forth: false  # disable workspace back and forth globally
```

## Schema

```yaml
workspace_back_and_forth: <boolean>
```

## Properties

### `workspace_back_and_forth`

:   <small>required</small> **type:** Boolean

    When `true`, navigating to the currently active workspace switches to the last focused workspace. When `false`, navigating to the current workspace does nothing.

## Default
```yaml
workspace_back_and_forth: true
```
