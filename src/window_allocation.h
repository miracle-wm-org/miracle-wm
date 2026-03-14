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

#ifndef MIRACLE_WINDOW_ALLOCATION_H
#define MIRACLE_WINDOW_ALLOCATION_H

#include "plugin_manager.h"

namespace miracle
{
class ParentContainer;
class AbstractWorkspace;

enum class AllocationType
{
    /// Allow the system to place the window, most likely in the tiling grid.
    grid,

    /// A shell window. These windows are shell components who are not associated
    /// with any one workspace, but may be associated with an output. Like freestyle
    /// windows, their life is determined by the client.
    shell,

    /// A window that will be entirely managed by a plugin.
    plugin,

    /// A freestyle window is one whose entire life is determined by the client.
    /// These windows remain on the current desktop or the desktop of their
    /// parent. This is intended for things like child windows, satellite windows,
    /// etc.
    freestyle,

    /// Error state.
    none
};

struct AllocationHint
{
    AllocationType container_type = AllocationType::none;
    ParentContainer* parent = nullptr;
    AbstractWorkspace* workspace = nullptr;
    PluginHandle plugin_handle = 0;
    glm::mat4 transform = glm::mat4(1.f);
    float alpha = 1.f;
    bool resizable = true;
    bool movable = true;
    uint64_t pending_window_id = 0;
};
}

#endif