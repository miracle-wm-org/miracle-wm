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

#include "window_manager_tools_tiling_interface.h"
#include "leaf_node.h"
#include "window_helpers.h"
#include "window_metadata.h"
#include "animator.h"

using namespace miracle;

WindowManagerToolsTilingInterface::WindowManagerToolsTilingInterface(
    miral::WindowManagerTools const& tools,
    Animator& animator) :
    tools { tools },
    animator { animator }
{
}

bool WindowManagerToolsTilingInterface::is_fullscreen(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    return window_helpers::is_window_fullscreen(info.state());
}

void WindowManagerToolsTilingInterface::set_rectangle(miral::Window const& window, geom::Rectangle const& r)
{
    auto old_rectangle = geom::Rectangle(window.top_left(), window.size());
    animator.animate_window_movement(
        window,
        old_rectangle,
        r
    );
}

MirWindowState WindowManagerToolsTilingInterface::get_state(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    return window_info.state();
}

void WindowManagerToolsTilingInterface::change_state(miral::Window const& window, MirWindowState state)
{
    auto& window_info = tools.info_for(window);
    miral::WindowSpecification spec;
    spec.state() = state;
    tools.place_and_size_for_state(spec, window_info);
    tools.modify_window(window, spec);
}

void WindowManagerToolsTilingInterface::clip(miral::Window const& window, geom::Rectangle const& r)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(r);
}

void WindowManagerToolsTilingInterface::noclip(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(mir::optional_value<geom::Rectangle>());
}

void WindowManagerToolsTilingInterface::select_active_window(miral::Window const& window)
{
    tools.select_active_window(window);
}

std::shared_ptr<WindowMetadata> WindowManagerToolsTilingInterface::get_metadata(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    if (info.userdata())
        return static_pointer_cast<WindowMetadata>(info.userdata());

    return nullptr;
}

std::shared_ptr<WindowMetadata> WindowManagerToolsTilingInterface::get_metadata(
    miral::Window const& window, TilingWindowTree const* tree)
{
    auto node = get_metadata(window);
    if (auto tiling_node = node->get_tiling_node())
    {
        if (tiling_node->get_tree() == tree)
            return node;
    }

    return nullptr;
}

void WindowManagerToolsTilingInterface::raise(miral::Window const& window)
{
    tools.raise_tree(window);
}

void WindowManagerToolsTilingInterface::send_to_back(miral::Window const& window)
{
    tools.send_tree_to_back(window);
}