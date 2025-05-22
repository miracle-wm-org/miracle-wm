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

    // When we resize the container, we're actually resizing the parent
    auto const parent = container->get_parent().lock();
    if (parent->anchored())
    {
        stop();
        return false;
    }

    // If the parent has more than 1 node suddenly, we need to stop resizing.
    if (parent->num_nodes() != 1)
    {
        stop();
        return false;
    }

    auto const rect = parent->get_area();
    int const current_x = rect.top_left.x.as_int();
    int const current_y = rect.top_left.y.as_int();
    int const current_width = rect.size.width.as_int();
    int const current_height = rect.size.height.as_int();

    // Calculate new size to keep edge at cursor position
    int new_x = current_x;
    int new_y = current_y;
    int new_width = current_width;
    int new_height = current_height;

    switch (resize_edge)
    {
    case mir_resize_edge_north:
        new_height = current_y + current_height - static_cast<int>(y);
        new_y = current_y + (current_height - new_height);
        break;
    case mir_resize_edge_south:
        new_height = static_cast<int>(y) - current_y;
        break;
    case mir_resize_edge_east:
        new_width = static_cast<int>(x) - current_x;
        break;
    case mir_resize_edge_west:
        new_width = current_x + current_width - static_cast<int>(x);
        new_x = current_x + (current_width - new_width);
        break;
    case mir_resize_edge_northeast:
        new_width = static_cast<int>(x) - current_x;
        new_height = current_y + current_height - static_cast<int>(y);
        new_y = current_y + (current_height - new_height);
        break;
    case mir_resize_edge_northwest:
        new_width = current_x + current_width - static_cast<int>(x);
        new_height = current_y + current_height - static_cast<int>(y);
        new_x = current_x + (current_width - new_width);
        new_y = current_y + (current_height - new_height);
        break;
    case mir_resize_edge_southeast:
        new_width = static_cast<int>(x) - current_x;
        new_height = static_cast<int>(y) - current_y;
        break;
    case mir_resize_edge_southwest:
        new_width = current_x + current_width - static_cast<int>(x);
        new_height = static_cast<int>(y) - current_y;
        new_x = current_x + (current_width - new_width);
        break;
    default:
        return false;
    }

    constexpr int MIN_SIZE = 50;
    if (new_width < MIN_SIZE)
    {
        new_width = MIN_SIZE;
        new_x = current_x;
    }
    if (new_height < MIN_SIZE)
    {
        new_height = MIN_SIZE;
        new_y = current_y;
    }

    // Set new size
    parent->set_logical_area(geom::Rectangle(
                                 geom::Point(new_x, new_y),
                                 geom::Size(new_width, new_height)),
        false);
    parent->commit_changes();

    return true;
}

void ResizeService::handle_request_resize(std::shared_ptr<Container> const& container, MirPointerAction action, MirResizeEdge edge)
{
    // Only leaf containers can be resized
    if (container->get_type() != ContainerType::leaf)
        return;

    // Only unanchored containers can be resized
    if (container->anchored())
        return;

    auto const parent = container->get_parent().lock();
    if (!parent)
        return;

    // Only containers that exist in their own tiling grid can be resized for now
    // TODO: Allow all containers to be resized, but this is trickier
    if (parent->get_parent().lock() != nullptr || parent->num_nodes() != 1)
        return;

    if (action == mir_pointer_action_button_down)
    {
        is_resizing = true;
        resizing_container = container;
        resize_edge = edge;
        command_controller->set_mode(WindowManagerMode::resizing);
    }
}

} // namespace miracle
