# Wobbly Windows

A miracle-wm plugin that recreates the classic "wobbly windows" effect from the
Compiz / Unity8 era: while a window is being moved it jiggles like jelly, then
settles back to rest once it stops.

It is also the reference example for the **per-window geometry shader** API
(`miracle_register_window_geometry_shader` / `Window::set_geometry_shader`).

## How it works

When a window starts moving, the plugin attaches a geometry shader to it
(`window.set_geometry_shader(Some(id))`). The shader subdivides the window quad
into a grid and displaces the generated vertices with a velocity-driven,
time-varying sine wave. The compositor feeds the shader two built-in uniforms:

- `u_time` — seconds since the compositor started.
- `u_velocity` — the window's center velocity in px/s.

Because the amplitude scales with `u_velocity`, the wobble fades to an identity
transform as soon as the window stops moving. A short settle timer then detaches
the shader.

## Requirements

Geometry shaders require a **GLSL ES 3.20 (OpenGL ES 3.2)** context. On hardware /
drivers without geometry-shader support the compositor logs a warning and renders
windows normally — the effect is simply a no-op.

## Building

```sh
rustup target add wasm32-wasip1
cargo build --target wasm32-wasip1 --release
```

The resulting module is at
`target/wasm32-wasip1/release/wobbly_windows.wasm`.

## Using

Point your miracle-wm config at the built module:

```yaml
# ~/.config/miracle-wm/config.yaml
plugins:
    - path: /absolute/path/to/wobbly_windows.wasm
```

Restart miracle-wm and drag a window to see it wobble.
