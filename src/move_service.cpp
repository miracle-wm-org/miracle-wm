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

#define MIR_LOG_COMPONENT "move_service"
#include "move_service.h"
#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "output_manager.h"
#include <mir/log.h>

using namespace miracle;

MoveService::MoveService(
    std::shared_ptr<AbstractCommandController> const& command_controller,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<OutputManager> const& output_manager) :
    command_controller(command_controller),
    config(config),
    output_manager(output_manager)
{
}

bool MoveService::handle_pointer_event(
    CompositorState& state,
    float x,
    float y,
    MirPointerAction action,
    unsigned int modifiers,
    MirPointerButtons buttons)
{
    std::lock_guard lock { mutex_ };
    if (state.mode() == WindowManagerMode::moving)
    {
        if (action == mir_pointer_action_button_up && (config->get_primary_button() & buttons) == 0)
        {
            command_controller->set_mode(WindowManagerMode::normal);
            return false;
        }

        if (!state.focused_container())
        {
            mir::log_warning("handle_pointer_event: focused container no longer exists while dragging");
            return false;
        }

        auto const dx = static_cast<int>(ceilf(x - cursor_x));
        auto const dy = static_cast<int>(ceilf(y - cursor_y));
        state.focused_container()->move_to(start_x + dx, start_y + dy, false);
        return true;
    }
    else if (action == mir_pointer_action_button_down)
    {
        if ((config->get_primary_button() & buttons) == 0)
            return false;

        uint const move_modifier = config->process_modifier(config->move_modifier());
        if (move_modifier != modifiers)
            return false;

        if (state.mode() != WindowManagerMode::normal)
        {
            mir::log_warning("Must be in normal mode before we can start moving");
            return false;
        }

        if (output_manager->focused() == nullptr)
            return false;

        auto const intersected = output_manager->focused()->intersect(x, y);
        if (!intersected)
            return false;

        if (intersected != state.focused_container())
            return false;

        command_controller->set_mode(WindowManagerMode::moving);
        command_controller->select_container(intersected);
        cursor_x = x;
        cursor_y = y;
        start_x = intersected->get_logical_area().top_left.x.as_int();
        start_y = intersected->get_logical_area().top_left.y.as_int();
        return true;
    }

    return false;
}
