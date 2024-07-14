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
#include <miral/window.h>

namespace miracle
{
enum class WindowManagerMode
{
    normal = 0,

    /// While resizing, only the window that was selected during
    /// resize can be selected. If that window closes, resize
    /// is completed.
    resizing
};

struct CompositorState
{
    mir::geometry::Point cursor_position;
    WindowManagerMode mode = WindowManagerMode::normal;
    miral::Window active_window;
};
}

#endif // MIRACLE_WM_COMPOSITOR_STATE_H
