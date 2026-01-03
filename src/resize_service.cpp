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

    Container::execute_resize(container.get(), resize_edge, x, y);
}

void ResizeService::handle_request_resize(std::shared_ptr<Container> const& container, MirPointerAction action, MirResizeEdge edge)
{
    if (action == mir_pointer_action_button_down && !is_resizing)
    {
        is_resizing = true;
        resizing_container = container;
        resize_edge = edge;
        command_controller->set_mode(WindowManagerMode::resizing);
    }
}

} // namespace miracle
