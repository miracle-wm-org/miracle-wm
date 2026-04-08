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

#include "resize_service.h"

#include "parent_container.h"

namespace miracle
{

ResizeService::ResizeService(
    std::shared_ptr<AbstractCommandController> const& command_controller,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<OutputManager> const& output_manager) :
    command_controller { command_controller },
    config { config },
    state { state },
    output_manager { output_manager }
{
}

void ResizeService::stop()
{
    is_resizing = false;
    last_x = 0;
    last_y = 0;
    resize_top_left = {};
    resize_size = {};
    command_controller->set_mode(WindowManagerMode::normal);
}

bool ResizeService::handle_pointer_event(float x, float y, MirPointerAction action)
{
    std::lock_guard lock { mutex_ };
    if (!is_resizing)
        return false;

    if (action == mir_pointer_action_button_up)
    {
        stop();
        return false;
    }

    // Only process motion events (ignore button_down, etc.)
    if (action != mir_pointer_action_motion)
        return false;

    auto const container = resizing_container.lock();
    if (!container)
    {
        stop();
        return false;
    }

    float const dx = x - last_x;
    float const dy = y - last_y;
    last_x = x;
    last_y = y;

    if (dx == 0 && dy == 0)
        return true;

    if (!container->anchored())
    {
        // Floating window: use miral-style accumulated position+size.
        // Never re-read from the constrained window state — always accumulate
        // from requested values to avoid position drift caused by constrain_resize.
        auto new_size = resize_size;
        auto new_pos = resize_top_left;

        if (resize_edge & mir_resize_edge_east)
            new_size.width = geom::Width { new_size.width.as_int() + static_cast<int>(dx) };
        if (resize_edge & mir_resize_edge_west)
        {
            new_size.width = geom::Width { new_size.width.as_int() - static_cast<int>(dx) };
            new_pos.x = geom::X { new_pos.x.as_int() + static_cast<int>(dx) };
        }
        if (resize_edge & mir_resize_edge_south)
            new_size.height = geom::Height { new_size.height.as_int() + static_cast<int>(dy) };
        if (resize_edge & mir_resize_edge_north)
        {
            new_size.height = geom::Height { new_size.height.as_int() - static_cast<int>(dy) };
            new_pos.y = geom::Y { new_pos.y.as_int() + static_cast<int>(dy) };
        }

        // Clamp to minimum size
        int const min_w = static_cast<int>(container->get_min_width());
        int const min_h = static_cast<int>(container->get_min_height());
        if (new_size.width.as_int() < min_w)
        {
            new_size.width = geom::Width { min_w };
            new_pos.x = resize_top_left.x;
        }
        if (new_size.height.as_int() < min_h)
        {
            new_size.height = geom::Height { min_h };
            new_pos.y = resize_top_left.y;
        }

        container->set_logical_area({ new_pos, new_size }, false);
        container->commit_changes();
        resize_top_left = new_pos;
        resize_size = new_size;
    }
    else
    {
        // Tiled window: use pure cursor delta through execute_resize.
        // LeafContainer caches the requested logical area, so there is no
        // constrain feedback loop — pure delta is safe and correct here.
        float x_diff = 0;
        float y_diff = 0;
        if (resize_edge & mir_resize_edge_east)
            x_diff = dx;
        if (resize_edge & mir_resize_edge_west)
            x_diff = -dx;
        if (resize_edge & mir_resize_edge_south)
            y_diff = dy;
        if (resize_edge & mir_resize_edge_north)
            y_diff = -dy;

        Container::execute_resize(container.get(), resize_edge, x_diff, y_diff, false);
    }

    return true;
}

void ResizeService::handle_request_resize(std::shared_ptr<WindowContainer> const& container, MirPointerAction action, MirResizeEdge edge, float x, float y)
{
    std::lock_guard lock { mutex_ };
    if (action == mir_pointer_action_button_down && !is_resizing)
    {
        is_resizing = true;
        resizing_container = container;
        resize_edge = edge;
        last_x = x;
        last_y = y;
        auto const rect = container->get_logical_area();
        resize_top_left = rect.top_left;
        resize_size = rect.size;
        command_controller->set_mode(WindowManagerMode::resizing);
    }
}

} // namespace miracle
