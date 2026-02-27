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

#define MIR_LOG_COMPONENT "drag_and_drop_service"

#include "drag_and_drop_service.h"
#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "constants.h"
#include "feature_flags.h"
#include "geometry_helpers.h"
#include "output_manager.h"
#include "window_container.h"

#include <mir/log.h>
#include <miral/toolkit_event.h>

using namespace miracle;

DragAndDropService::DragAndDropService(
    std::shared_ptr<AbstractCommandController> const& command_controller,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<OutputManager> const& output_manager) :
    command_controller { command_controller },
    config { config },
    output_manager { output_manager }
{
}

bool DragAndDropService::handle_pointer_event(CompositorState& state, float x, float y, MirPointerAction action, unsigned int modifiers, MirPointerButtons buttons)
{
    if (!config->drag_and_drop().enabled)
        return false;

    auto const window_container = std::dynamic_pointer_cast<WindowContainer>(state.focused_container());

    if (state.mode() == WindowManagerMode::dragging)
    {
        if (action == mir_pointer_action_button_up && (config->get_primary_button() & buttons) == 0)
        {
            command_controller->set_mode(WindowManagerMode::normal);
            if (window_container)
                window_container->drag_stop();
            last_intersected.reset();
            return true;
        }

        if (!state.focused_container())
        {
            mir::log_warning("handle_drag_and_drop_pointer_event: focused container no longer exists while dragging");
            return false;
        }

        /// If we haven't moved since last time, there's nothing to do
        if (current_x == x && current_y == y)
            return true;

        current_x = x;
        current_y = y;

        // Drag the container to the new position
        float const diff_x = x - cursor_start_x;
        float const diff_y = y - cursor_start_y;
        window_container->drag(
            static_cast<int>(container_start_x + diff_x),
            static_cast<int>(container_start_y + diff_y));

        if (output_manager->focused()->active()->is_empty())
        {
            drag_to(window_container, output_manager->focused()->active().get());
            return true;
        }

        // Get the intersection and try to move ourselves there. We only care if we're intersecting
        // a leaf container, as those would be the only one in the grid.
        std::shared_ptr<WindowContainer> intersected = output_manager->focused()->intersect_leaf(x, y, true);
        if (!intersected)
        {
            last_intersected.reset();
            return true;
        }

        if (last_intersected.lock() == intersected)
            return true;

        last_intersected = intersected;
        drag_to(window_container, intersected);
        return true;
    }
    else if (action == mir_pointer_action_button_down)
    {
        if ((config->get_primary_button() & buttons) == 0)
            return false;

        uint command_modifiers = config->process_modifier(config->drag_and_drop().modifiers);
        if (command_modifiers != modifiers)
            return false;

        if (state.mode() != WindowManagerMode::normal)
        {
            mir::log_warning("Must be in normal mode before we can start dragging");
            return false;
        }

        if (output_manager->focused() == nullptr)
            return false;

        auto intersected_container = output_manager->focused()->intersect(x, y);
        if (!intersected_container)
            return false;

        auto intersected = Container::as_window_container(intersected_container);
        if (!intersected || !intersected->drag_start())
        {
            mir::log_warning("Cannot drag container of type %d", (int)intersected_container->get_type());
            return false;
        }

        command_controller->set_mode(WindowManagerMode::dragging);
        command_controller->select_container(intersected);
        cursor_start_x = x;
        cursor_start_y = y;
        container_start_x = miracle::geometry_helpers::gl::x(intersected->get_visible_area().top_left);
        container_start_y = miracle::geometry_helpers::gl::y(intersected->get_visible_area().top_left);
        current_x = x;
        current_y = y;
        return true;
    }

    return false;
}

void DragAndDropService::stop_drag(CompositorState const& state)
{
    command_controller->set_mode(WindowManagerMode::normal);
    auto const window_container = std::dynamic_pointer_cast<WindowContainer>(state.focused_container());
    if (window_container)
        window_container->drag_stop();
}

void DragAndDropService::drag_to(
    std::shared_ptr<WindowContainer> const& dragging,
    std::shared_ptr<WindowContainer> const& to)
{
    if (dragging == to)
        return;

    if (dragging)
        dragging->move_to(*to);
}

void DragAndDropService::drag_to(
    std::shared_ptr<WindowContainer> const& dragging,
    WorkspaceInterface* workspace)
{
    if (dragging->get_workspace().get() == workspace)
        return;

    workspace->add_to_root(*dragging);
}
