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

#ifndef MIRACLEWM_SURFACE_TRACKER_H
#define MIRACLEWM_SURFACE_TRACKER_H

#include <cstdint>
#include <glm/glm.hpp>
#include <mir/scene/surface.h>
#include <miral/window.h>
#include <mutex>
#include <vector>

namespace miracle
{

class Container;
typedef int RenderDataManagerId;

/// What an in-flight resize animation wants done to a window's drawn content.
struct ContentStretch
{
    /// The size to draw the window's content at this frame. Never a size the client
    /// cannot reach: once the animated clip passes the client's limit this freezes at
    /// that limit and the clip crops the difference - which is exactly what the
    /// un-animated frame after it does, so switching the stretch off is invisible.
    mir::geometry::Size size;

    /// The window size the animation started from - an identity token for the animation,
    /// not a measurement. The renderer holds on to the frame a window was showing when
    /// its resize began so it can cross-fade the live surface in over it, and a changed
    /// [from] is what tells it the animation was retargeted mid-flight and the frame it
    /// is holding is no longer the one the user was looking at.
    mir::geometry::Size from;

    /// How much of the pre-resize frame the renderer should still be showing. 0 means the
    /// live surface is drawn alone, which is every frame of a pure move and every frame
    /// after the cross-fade has finished.
    float fade = 0.f;

    friend bool operator==(ContentStretch const&, ContentStretch const&) = default;
};

struct RenderData
{
    RenderDataManagerId id = 0;
    /// The window whose surface this data describes. Holds the surface only
    /// weakly, so it does not extend the surface's lifetime.
    miral::Window window;
    bool needs_outline = false;
    bool is_focused = false;
    glm::mat4 transform = glm::mat4(1.f);
    glm::mat4 workspace_transform = glm::mat4(1.f);
    std::optional<mir::geometry::Rectangle> output_area;
    std::optional<uint8_t> shader_id = std::nullopt;
    /// What an in-flight resize animation wants done to this window's content, or
    /// nothing when it should be drawn at its own size.
    std::optional<ContentStretch> stretch;
};

class RenderDataManager
{
public:
    RenderDataManager();
    RenderDataManagerId add(RenderData const&&);
    void remove(RenderDataManagerId id);
    void transform_change(RenderDataManagerId id, glm::mat4 const& transform);
    void workspace_transform_change(RenderDataManagerId id, glm::mat4 const& transform);
    void output_area_change(RenderDataManagerId id, mir::geometry::Rectangle const& area);
    void focus_change(RenderDataManagerId id, bool is_focused);
    void needs_outline_change(RenderDataManagerId id, bool needs_outline);
    void shader_id_change(RenderDataManagerId id, std::optional<uint8_t> shader_id);
    void stretch_change(RenderDataManagerId id, std::optional<ContentStretch> stretch);
    /// Reset every RenderData whose shader_id is in \p ids back to the default
    /// shader (std::nullopt). Used when the shaders are removed (e.g. on plugin unload).
    void reset_shaders(std::vector<uint8_t> const& ids);
    /// Copies the current render data into \p out and updates \p seen_generation,
    /// or does nothing if no data has changed since \p seen_generation. Callers on
    /// different threads must each provide their own \p seen_generation and \p out.
    void copy_if_changed(uint64_t& seen_generation, std::vector<RenderData>& out);

private:
    RenderDataManagerId next_id = 0;
    uint64_t generation = 1;
    std::mutex mutex;
    /// Sorted by ascending id: add() assigns monotonically increasing ids
    /// and remove() preserves order.
    std::vector<RenderData> render_data;
};

} // miracle

#endif // MIRACLEWM_SURFACE_TRACKER_H
