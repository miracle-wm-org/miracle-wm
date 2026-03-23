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

    auto const container = resizing_container.lock();
    if (!container)
    {
        stop();
        return false;
    }

    float x_diff = 0;
    float y_diff = 0;
    auto const area = container->get_visible_area();
    switch (resize_edge)
    {
    case mir_resize_edge_west:
        x_diff = area.left().as_value() - x;
        break;
    case mir_resize_edge_east:
        x_diff = x - area.right().as_value();
        break;
    case mir_resize_edge_south:
        y_diff = y - area.bottom().as_value();
        break;
    case mir_resize_edge_north:
        y_diff = area.top().as_value() - y;
        break;
    default:
        break;
    }

    Container::execute_resize(container.get(), resize_edge, x_diff, y_diff, false);
    return true;
}

void ResizeService::handle_request_resize(std::shared_ptr<WindowContainer> const& container, MirPointerAction action, MirResizeEdge edge)
{
    std::lock_guard lock { mutex_ };
    if (action == mir_pointer_action_button_down && !is_resizing)
    {
        is_resizing = true;
        resizing_container = container;
        resize_edge = edge;
        command_controller->set_mode(WindowManagerMode::resizing);
    }
}

} // namespace miracle
