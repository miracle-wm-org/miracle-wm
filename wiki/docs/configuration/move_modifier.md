# Move Modifier
While the specified modifier is held and the user clicks the primary mouse
button, they will be able to translate the container 
under the pointer to the resulting position of the cursor. If this
container belongs to a group of containers (e.g. a floating mini tree), then
the entire tree will be translated.

## Example
```yaml
# ~/.config/miracle-wm/config.yaml

move_modifier:
  - primary
  - shift
```

## Schema

```yaml
move_modifier: <Modifier[]>
```

## Properties

### `move_modifier`

:   <small>required</small> **type:** List of modifier keys

    Modifier keys that must be held while clicking the primary mouse button to translate (move) the container under the pointer. If the container belongs to a group (e.g., a floating mini tree), the entire tree will be translated.
    
    For a list of valid modifiers, see [Action Key](action_key.md).

## Default
```yaml
move_modifier:
  - primary
```
