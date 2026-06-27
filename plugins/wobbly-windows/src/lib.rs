//! Wobbly windows for miracle-wm.
//!
//! Recreates the "wobbly windows" effect from the Compiz / Unity8 era: while a
//! window is being moved it jiggles like a sheet of jelly, then settles back to
//! rest once it stops.
//!
//! It works by attaching a per-window **geometry shader** (see
//! [`miracle_plugin::register_window_geometry_shader`]) to the window that is being
//! moved. The geometry shader subdivides the window quad into a grid and displaces
//! the generated vertices with a velocity-driven, time-varying sine wave. The
//! compositor feeds the shader two built-in uniforms — `u_time` (seconds) and
//! `u_velocity` (the window's center velocity in px/s) — so the wobble's amplitude
//! tracks how fast the window is dragged and naturally decays to zero (an identity
//! transform) once motion stops.
//!
//! Geometry shaders require a GLSL ES 3.20 (OpenGL ES 3.2) context. On systems
//! without geometry-shader support the compositor logs a warning and renders the
//! window normally — the effect is simply a no-op.

use std::cell::RefCell;
use std::collections::HashSet;

use miracle_plugin::animation::{AnimationFrameData, AnimationFrameResult};
use miracle_plugin::host::miracle_window_set_geometry_shader_id;
use miracle_plugin::plugin::Plugin;
use miracle_plugin::window::Window;
use miracle_plugin::{queue_custom_animation, register_window_geometry_shader};

/// The geometry shader implementing the wobble.
///
/// Contract (see the `register_window_geometry_shader` docs):
/// - input: `in vec2 v_texcoord[]` and `gl_in[].gl_Position` from the vertex stage;
/// - output: `out vec2 g_texcoord` and `gl_Position` to the fragment stage;
/// - built-in uniforms `u_time`, `u_velocity`, `surfaceSize` supplied by the host.
const WOBBLE_GEOMETRY_SHADER: &str = r#"#version 320 es
layout(triangles) in;
layout(triangle_strip, max_vertices = 64) out;

in vec2 v_texcoord[];
out vec2 g_texcoord;

uniform float u_time;
uniform vec2 u_velocity;

// Displace a clip-space position based on the window-relative texcoord, the elapsed
// time and the window's velocity. The amplitude scales with speed and the corners
// lag behind the center, giving the classic jelly wobble.
vec4 wobble(vec4 clip, vec2 tc) {
    float speed = length(u_velocity);
    // Map speed (px/s) to a normalized-device-coordinate amplitude, capped so fast
    // flicks don't fold the window inside out.
    float amp = min(speed / 3000.0, 1.0) * 0.05;
    float t = u_time * 10.0;

    // A ripple across the surface so the whole window jiggles, not just the edges.
    vec2 o;
    o.x = sin(t + tc.y * 12.0) * amp;
    o.y = cos(t + tc.x * 12.0) * amp;

    // Bias the displacement opposite the direction of motion and weight it toward
    // the window's edges, so the trailing edge drags behind.
    vec2 dir = speed > 1.0 ? -u_velocity / speed : vec2(0.0);
    float edge = abs(tc.x - 0.5) + abs(tc.y - 0.5);

    clip.xy += (o + dir * amp * edge) * clip.w;
    return clip;
}

void emit(float u, float v) {
    float w = 1.0 - u - v;
    vec4 p = u * gl_in[1].gl_Position
           + v * gl_in[2].gl_Position
           + w * gl_in[0].gl_Position;
    vec2 tc = u * v_texcoord[1] + v * v_texcoord[2] + w * v_texcoord[0];
    gl_Position = wobble(p, tc);
    g_texcoord = tc;
    EmitVertex();
}

void main() {
    // Subdivide the incoming triangle into an LxL grid of smaller triangles using
    // barycentric coordinates, so the displacement above varies smoothly across the
    // window surface. L = 4 keeps the emitted vertex count (<= 48) within budget.
    const int L = 4;
    for (int i = 0; i < L; ++i) {
        for (int j = 0; j < L - i; ++j) {
            float u0 = float(i) / float(L);
            float u1 = float(i + 1) / float(L);
            float v0 = float(j) / float(L);
            float v1 = float(j + 1) / float(L);

            emit(u0, v0);
            emit(u1, v0);
            emit(u0, v1);
            EndPrimitive();

            if (j < L - i - 1) {
                emit(u1, v0);
                emit(u1, v1);
                emit(u0, v1);
                EndPrimitive();
            }
        }
    }
}
"#;

/// Seconds after the last move frame before we detach the geometry shader. The
/// renderer's velocity uniform decays the wobble to rest well within this window;
/// detaching afterwards stops the (otherwise idle) geometry stage from running.
const SETTLE_SECONDS: f32 = 0.7;

#[derive(Default)]
struct WobblyState {
    /// Lazily-registered geometry shader id (registration is attempted once).
    geom_id: Option<u8>,
    registered: bool,
    /// Windows that already have a pending settle timer, so we don't queue one per
    /// frame for the duration of a drag.
    armed: HashSet<u64>,
}

impl WobblyState {
    fn ensure_geometry_shader(&mut self) -> Option<u8> {
        if !self.registered {
            self.registered = true;
            self.geom_id = register_window_geometry_shader(WOBBLE_GEOMETRY_SHADER);
        }
        self.geom_id
    }
}

thread_local! {
    // Plugins run single-threaded inside the WASM sandbox, so a thread-local cell is
    // a simple way to share state between the move hook and the settle-timer closure.
    static STATE: RefCell<WobblyState> = RefCell::new(WobblyState::default());
}

#[derive(Default)]
struct WobblyWindows;

impl Plugin for WobblyWindows {
    fn window_move_animation(
        &mut self,
        _data: &AnimationFrameData,
        window: &Window,
    ) -> Option<AnimationFrameResult> {
        let wid = window.id();

        let geom_id = STATE.with(|s| s.borrow_mut().ensure_geometry_shader());
        let Some(geom_id) = geom_id else {
            // Registration failed; nothing we can do.
            return None;
        };

        // Attach the wobble shader for the duration of the move. Setting it every
        // frame is cheap and idempotent (the renderer caches the compiled program).
        let _ = window.set_geometry_shader(Some(geom_id));

        // Arm a one-shot settle timer the first time we see this window move. When it
        // fires we detach the shader; if the window is still moving by then, the next
        // move frame simply re-arms and re-attaches.
        let needs_timer = STATE.with(|s| s.borrow_mut().armed.insert(wid));
        if needs_timer {
            queue_custom_animation(
                move |_id, _dt, elapsed| {
                    if elapsed >= SETTLE_SECONDS {
                        unsafe {
                            miracle_window_set_geometry_shader_id(wid as i64, -1);
                        }
                        STATE.with(|s| {
                            s.borrow_mut().armed.remove(&wid);
                        });
                    }
                },
                SETTLE_SECONDS,
            );
        }

        // Return None: we only add the wobble; the compositor still drives the
        // window's actual movement with its normal move animation.
        None
    }
}

miracle_plugin::miracle_plugin!(WobblyWindows);
