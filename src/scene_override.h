/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef SCENE_OVERRIDE_H
#define SCENE_OVERRIDE_H

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miral/window.h>
#include <mutex>
#include <optional>
#include <vector>

namespace mir::scene
{
class Surface;
}

namespace miracle
{

class SceneOverride;

/// Manages scene overrides.
///
/// While the scene is being overridden, input and rendering will be deferred
/// to the [SceneOverride] until exited.
///
/// Only a single override can be active at any one time.
class SceneOverrideManager
{
public:
    SceneOverrideManager() = default;

    /// Attempt to override the scene with \p override.
    ///
    /// \returns the override token, or std::nullopt if the scene cannot be overridden.
    std::optional<uint32_t> try_override(std::unique_ptr<SceneOverride> override);

    /// Attempt to release the override associated with \p token.
    ///
    /// The override may be destroyed on whichever thread drops the last
    /// reference to it (e.g. the render thread), so overrides must have
    /// thread-agnostic destructors.
    bool try_release_override(uint32_t token);

    /// Retrieves the current scene override, if any.
    ///
    /// Returns nullptr if none exists.
    ///
    /// The returned pointer pins the override: holding it guarantees the
    /// override stays alive even if another thread releases it in the meantime.
    std::shared_ptr<SceneOverride> try_resolve();

private:
    std::mutex mutex;
    uint32_t next_token = 0;
    uint32_t current_token = 0;
    std::shared_ptr<SceneOverride> current;
};

struct SceneOverridePlacement
{
    /// Where the window's top-left corner is moved to, in logical screen
    /// coordinates (the same space as the window's real screen position).
    ///
    /// The window keeps its real size: an override that wants it to appear
    /// smaller or larger expresses that as a scale in [transformation], so the
    /// surface is never clipped to fit.
    glm::vec2 position;

    /// Additional transformation, applied about the center of the relocated
    /// window, exactly like the normal per-window transform.
    glm::mat4 transformation;

    /// Multiplied into the surface's own alpha while the override is active,
    /// so that an override can dim a window without disturbing the container's
    /// workspace, window and animation alpha layers.
    float opacity = 1.f;

    /// The output this placement belongs to, in logical screen coordinates.
    ///
    /// The placement is never drawn on any other output, so an override whose
    /// layout deliberately runs off the edges of one output - a carousel with
    /// its neighbours hanging past both sides - does not spill onto the next
    /// output along. Whatever hangs off the edge is clipped by that output's
    /// own framebuffer, so the result is exact.
    ///
    /// nullopt draws the placement wherever it lands, on every output it
    /// touches.
    std::optional<mir::geometry::Rectangle> clip;
};

/// An override in how the scene is rendered.
///
/// While the scene is being overridden, window management will continue as normal
/// per Miracle's main policy. The override only applies at render time and input time.
/// When in an override scene, keyboard and mouse input are NOT routed
/// to the windows and instead should be managed by the scene itself.
///
/// The override placements will inherit the following from the window management policy:
/// * Output + workspace transform
/// * Shader
/// * Border
class SceneOverride
{
public:
    virtual ~SceneOverride() = default;

    /// Handle a keyboard event.
    ///
    /// All events are NOT forwarded to clients during a scene override, so you are
    /// guaranteed to be the only one who receives the event.
    ///
    /// This is called on the input thread under the scene lock.
    virtual void handle_keyboard_event(MirKeyboardEvent const* event) = 0;

    /// Handle a pointer event.
    ///
    /// All events are NOT forwarded to clients during a scene override, so you are
    /// guaranteed to be the only one who receives the event.
    ///
    /// This is called on the input thread under the scene lock.
    virtual void handle_pointer_event(MirPointerEvent const* event) = 0;

    /// Fill \p out with the placements at which \p surface should be drawn this
    /// frame.
    ///
    /// Leaving \p out empty means that the override does not manage this
    /// surface and it should be rendered exactly as normal. More than one
    /// placement draws the surface once per placement, which is how a single
    /// panel or wallpaper surface - there is only ever one of each per output -
    /// can appear on every tile of a workspace overview.
    ///
    /// An override that lays surfaces out per output should set
    /// [SceneOverridePlacement::clip] on every placement it produces, so that a
    /// layout which deliberately overflows one output does not bleed onto the
    /// one beside it.
    ///
    /// \p out is a scratch buffer owned by the renderer and reused between
    /// frames, so filling it costs no allocation in the steady state. It is
    /// cleared before the call.
    ///
    /// This is called on the render thread, so be careful! It runs once per
    /// surface group per frame, so it should be a lightweight (ideally cached!)
    /// operation.
    ///
    /// For example, if the [Animator] is busy updating the position, this call
    /// should simply grab the cached position that the animator created, rather
    /// than calling the animation itself.
    ///
    /// \param surface the surface about to be drawn
    /// \param real    where the surface really is, in logical screen coordinates
    virtual void place(
        mir::scene::Surface const& surface,
        mir::geometry::Rectangle const& real,
        std::vector<SceneOverridePlacement>& out)
        = 0;

    /// Called on the window management thread when a new window appears in the
    /// scene, so that the override can decide how to place it.
    virtual void handle_window_added(miral::Window const&) { }

    /// Called on the window management thread when a window is removed from the scene.
    virtual void handle_window_closed(miral::Window const&) { }

    /// Called on the window management thread whenever an output is created,
    /// updated, or removed. Output changes invalidate any cached geometry, so
    /// overrides are expected to cancel themselves immediately (no exit
    /// animation). This can happen frequently, so implementations must be
    /// cheap and idempotent.
    virtual void handle_output_changed() = 0;
};
}

#endif
