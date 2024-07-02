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

#ifndef MIRACLEWM_WINDOW_HELPERS_H
#define MIRACLEWM_WINDOW_HELPERS_H

#include "window_metadata.h"
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class LeafContainer;
class TilingWindowTree;

namespace window_helpers
{
    bool is_window_fullscreen(MirWindowState state);

    template <typename T>
    WindowType get_ideal_type(T const& requested_specification)
    {
        auto has_exclusive_rect = requested_specification.exclusive_rect().is_set();
        auto is_attached = requested_specification.attached_edges().is_set();
        if (has_exclusive_rect || is_attached)
            return WindowType::other;

        auto t = requested_specification.type();
        if (t == mir_window_type_normal || t == mir_window_type_freestyle)
        {
            return WindowType::none;
        }

        return WindowType::other;
    }

    std::shared_ptr<WindowMetadata> get_metadata(
        miral::WindowInfo const& info);

    std::shared_ptr<WindowMetadata> get_metadata(
        miral::Window const& window,
        miral::WindowManagerTools const& tools);

    std::shared_ptr<LeafContainer> get_node_for_window(
        miral::Window const& window,
        miral::WindowManagerTools const& tools);

    std::shared_ptr<LeafContainer> get_node_for_window_by_tree(
        miral::Window const& window,
        miral::WindowManagerTools const& tools,
        TilingWindowTree const* tree);

    miral::WindowSpecification copy_from(miral::WindowInfo const&);
}
}

#endif // MIRACLEWM_WINDOW_HELPERS_H
