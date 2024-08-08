/**
Copyright (C) 2024  Matthew Kosarek

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

#include <mir/geometry/point.h>
#include <memory>

namespace miracle
{
class Container;
class Output;

enum class WindowManagerMode
{
    normal = 0,

    /// While [resizing], only the window that was selected during
    /// resize can be selected. If that window closes, resize
    /// is completed.
    resizing,

    /// While [selecting], only [Container]s selected with the multi-select
    /// keybind/mousebind can be selected or deselected.
    selecting
};

struct CompositorState
{
    WindowManagerMode mode = WindowManagerMode::normal;
    std::shared_ptr<Output> active_output;
    std::shared_ptr<Container> active;
    mir::geometry::Point cursor_position;
    uint32_t modifiers = 0;
    bool has_clicked_floating_window = false;
};
}

#endif // MIRACLE_WM_COMPOSITOR_STATE_H
