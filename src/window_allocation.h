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
    system,
    shell,
    plugin,
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