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

#ifndef MIRACLE_WM_COMPOSITOR_STATE_H
#define MIRACLE_WM_COMPOSITOR_STATE_H

#include "container.h"
#include "render_data_manager.h"
#include "window_container.h"

#include <atomic>
#include <memory>
#include <mir/geometry/point.h>
#include <vector>

namespace miracle
{
class SceneOverrideManager;

enum class WindowManagerMode
{
    normal = 0,

    /// While [resizing], only the window that was selected during
    /// resize can be selected. If that window closes, resize
    /// is completed.
    resizing,

    dragging,

    moving,

    max
};

constexpr std::array<std::string, static_cast<int>(WindowManagerMode::max)> BINDING_MODE_STRINGS = {
    "default", "resize", "dragging", "moving"
};

class CompositorState
{
public:
    CompositorState();
    mir::geometry::Point cursor_position;

    [[nodiscard]] std::shared_ptr<Container> focused_container() const;

    /// Focus the provided \p container.
    ///
    /// The container does not necessarily need to be a [WindowContainer], but it often is.
    void focus_container(std::shared_ptr<Container> const& container);

    /// Unfocus the provided \p container
    void unfocus_container(std::shared_ptr<Container> const& container);

    /// Adds a new window to the focus list.
    ///
    /// \param container the window to add
    void add(std::shared_ptr<WindowContainer> const& container);

    /// Removes a window from the focus list.
    ///
    /// \param container the window to remove
    void remove(std::shared_ptr<WindowContainer> const& container);

    /// Finds the first floating window, if one exists.
    ///
    /// \returns the first floating window, if one exists
    [[nodiscard]] std::shared_ptr<WindowContainer> first_floating();

    /// Finds the first tiling window, if one exists.
    ///
    /// \returns the first tiling window, if one exists
    [[nodiscard]] std::shared_ptr<WindowContainer> first_tiling();

    /// Returns the list of windows in their focus order.
    ///
    /// \returns list of windows
    [[nodiscard]] std::vector<std::weak_ptr<WindowContainer>> windows() const { return focus_order; }

    WindowManagerMode mode();
    void mode(WindowManagerMode);
    std::shared_ptr<RenderDataManager> const& render_data_manager() const;
    std::shared_ptr<SceneOverrideManager> const& scene_override_manager() const;

    /// Returns the next unique container ID, then increments the counter.
    uint64_t next_container_id();

    /// Returns the next unique container ID but does NOT increment the counter.
    /// This is useful for `Policy::place_new_window` only!
    ///
    /// \returns the next container id
    uint64_t peak_next_container_id() const;

private:
    mutable std::mutex mutex;
    std::weak_ptr<Container> focused;
    std::vector<std::weak_ptr<WindowContainer>> focus_order;
    WindowManagerMode mode_ = WindowManagerMode::normal;
    std::shared_ptr<RenderDataManager> render_data_manager_;
    std::shared_ptr<SceneOverrideManager> scene_override_manager_;
    std::atomic<uint64_t> next_container_id_ { 0 };
};
}

#endif // MIRACLE_WM_COMPOSITOR_STATE_H
