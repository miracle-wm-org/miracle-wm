# wobbly-windows

A miracle-wm plugin that makes windows wobble as they slide to a new position,
then settle.

The compositor's built-in move animation still moves the window. The plugin reads
where that animation puts the window on every frame, drives a damped spring from
it, and hands the spring's lag to a geometry shader that bends the window (content
and border together).

## Requirements

- An OpenGL ES 3.2 context (geometry shaders). Without one the compositor logs a
  warning and windows move normally.
- Animations enabled, with a `window_move` animation configured (e.g. `slide`).

## Building

```sh
rustup target add wasm32-wasip1
cargo build --target wasm32-wasip1 --release
```

## Enabling

```yaml
plugins:
  - path: /path/to/wobbly_windows.wasm
```
