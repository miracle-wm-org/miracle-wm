# Animations
Miracle supports a number of different built-in animations that can
be configured by the user. If you would like to turn off animations
by default, please see [Enable Animations](./enable_animations.md).

## Examples
```yaml
# ~/.config/miracle-wm/config.yaml

# Override workspace switching to take 1s with linear interpolation
animations:
  - event: workspace_switch
    duration: 1
    type: built_in
    parts:
      - type: slide
        function: linear
```

```yaml
# Customize easing constants for a bounce effect on window open
animations:
  - event: window_open
    duration: 0.3
    type: built_in
    parts:
      - type: grow
        function: ease_out_bounce
        n1: 7.0
        d1: 2.5
```


## Schema

A list of animation rules. Each rule defines how a specific event should be animated.

```yaml
animations:
  - event: <window_open|window_move|window_close|workspace_switch>
    duration: <float seconds>
    type: <built_in>
    parts?:
      - type: <disabled|slide|grow|shrink|fade>
        function: <linear|ease_in_*|ease_out_*|ease_in_out_*>  # see https://easings.net/
        c1?: <float=1.2>
        c2?: <float=1.83>
        c3?: <float=2.2>
        c4?: <float=2.0943951023931953>
        c5?: <float=1.3962634015954636>
        n1?: <float=7.5625>
        d1?: <float=2.75>
```

## Properties

### `event`

:   <small>required</small> **type:** `window_open` | `window_move` | `window_close` | `workspace_switch`

    The window or workspace event that triggers this animation.

### `duration`

:   <small>required</small> **type:** Float (seconds)

    Total time the animation takes to complete. All parts run sequentially within this duration.

### `type`

:   <small>required</small> **type:** `built_in`

    The animation implementation:
    
    - `built_in` — Use one or more built-in animation parts (requires `parts`)

---

## Built-in Animations

For animations with `type: built_in`:

### `parts`

:   <small>required</small> **type:** List of animation parts

    Each part runs in sequence within the total `duration`. Properties per part:

#### `type`

:   <small>required</small> **type:** `disabled` | `slide` | `grow` | `shrink` | `fade`

    The animation effect to apply.

#### `function`

:   <small>required</small> **type:** Easing function name

    Easing function (see [easings.net](https://easings.net/))  
    Options: `linear`, `ease_in_sine`, `ease_out_sine`, `ease_in_out_sine`, `ease_in_quad`, `ease_out_quad`, `ease_in_out_quad`, `ease_in_cubic`, `ease_out_cubic`, `ease_in_out_cubic`, `ease_in_quart`, `ease_out_quart`, `ease_in_out_quart`, `ease_in_quint`, `ease_out_quint`, `ease_in_out_quint`, `ease_in_expo`, `ease_out_expo`, `ease_in_out_expo`, `ease_in_circ`, `ease_out_circ`, `ease_in_out_circ`, `ease_in_back`, `ease_out_back`, `ease_in_out_back`, `ease_in_elastic`, `ease_out_elastic`, `ease_in_out_elastic`, `ease_in_bounce`, `ease_out_bounce`, `ease_in_out_bounce`

#### `c1`

:   **type:** Float  
    **Default:** `1.2`

    Easing constant for back and elastic functions.

#### `c2`

:   **type:** Float  
    **Default:** `1.83`

    Easing constant for back functions.

#### `c3`

:   **type:** Float  
    **Default:** `2.2`

    Easing constant for back functions.

#### `c4`

:   **type:** Float  
    **Default:** `2.0943951023931953` (2π/3)

    Easing constant for elastic functions.

#### `c5`

:   **type:** Float  
    **Default:** `1.3962634015954636` (2π/4.5)

    Easing constant for elastic functions.

#### `n1`

:   **type:** Float  
    **Default:** `7.5625`

    Easing constant for bounce functions.

#### `d1`

:   **type:** Float  
    **Default:** `2.75`

    Easing constant for bounce functions.

---


## Default
```yaml
animations:
  - event: window_open
    duration: 0.25
    type: built_in
    parts:
      - type: grow
        function: ease_in_out_back
  - event: window_move
    duration: 0.25
    type: built_in
    parts:
      - type: slide
        function: ease_in_out_back
  - event: window_close
    duration: 0.25
    type: built_in
    parts:
      - type: shrink
        function: ease_out_back
  - event: workspace_switch
    duration: 0.175
    type: built_in
    parts:
      - type: slide
        function: ease_out_sine
```
