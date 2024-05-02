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

#include "i3_command_executor.h"
#include "leaf_node.h"
#include "parent_node.h"
#include "policy.h"
#include "window_helpers.h"

#define MIR_LOG_COMPONENT "miracle"
#include <mir/log.h>
#include <miral/application_info.h>

using namespace miracle;

I3CommandExecutor::I3CommandExecutor(
    miracle::Policy& policy,
    WorkspaceManager& workspace_manager,
    miral::WindowManagerTools const& tools) :
    policy { policy },
    workspace_manager { workspace_manager },
    tools { tools }
{
}

void I3CommandExecutor::process(miracle::I3ScopedCommandList const& command_list)
{
    for (auto const& command : command_list.commands)
    {
        switch (command.type)
        {
        case I3CommandType::focus:
            process_focus(command, command_list);
            break;
        default:
            break;
        }
    }
}

miral::Window I3CommandExecutor::get_window_meeting_criteria(I3ScopedCommandList const& command_list)
{
    miral::Window result;
    tools.find_application([&](miral::ApplicationInfo const& info)
    {
        for (auto const& window : info.windows())
        {
            if (command_list.meets_criteria(window, tools))
            {
                result = window;
                return true;
            }
        }

        return false;
    });
    return result;
}

void I3CommandExecutor::process_focus(I3Command const& command, I3ScopedCommandList const& command_list)
{
    auto active_output = policy.get_active_output();
    if (!active_output)
    {
        mir::log_warning("Trying to process I3 focus command, but output is not set");
        return;
    }

    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        if (command_list.scope.empty())
        {
            mir::log_warning("Focus command expected scope but none was provided");
            return;
        }

        auto window = get_window_meeting_criteria(command_list);
        if (window)
            active_output->select_window(window);

        return;
    }

    auto const& arg = command.arguments.front();
    if (arg == "workspace")
    {
        if (command_list.scope.empty())
        {
            mir::log_warning("Focus 'workspace' command expected scope but none was provided");
            return;
        }

        auto window = get_window_meeting_criteria(command_list);
        auto metadata = window_helpers::get_metadata(window, tools);
        if (metadata)
            workspace_manager.request_focus(metadata->get_workspace());
    }
    else if (arg == "left")
        active_output->select(Direction::left);
    else if (arg == "right")
        active_output->select(Direction::right);
    else if (arg == "up")
        active_output->select(Direction::up);
    else if (arg == "down")
        active_output->select(Direction::down);
    else if (arg == "parent")
        mir::log_warning("'focus parent' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "child")
        mir::log_warning("'focus child' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "prev")
    {
        auto active_window = tools.active_window();
        if (!active_window)
            return;

        auto metadata = window_helpers::get_metadata(active_window, tools);
        if (!metadata)
            return;

        if (metadata->get_type() != WindowType::tiled)
        {
            mir::log_warning("Cannot focus prev when a tiling window is not selected");
            return;
        }

        auto node = metadata->get_tiling_node();
        auto parent = node->get_parent().lock();
        auto index = parent->get_index_of_node(node);
        if (index != 0)
        {
            auto node_to_select = parent->get_nth_window(index - 1);
            active_output->select_window(node_to_select->get_window());
        }
    }
    else if (arg == "next")
    {
        auto active_window = tools.active_window();
        if (!active_window)
            return;

        auto metadata = window_helpers::get_metadata(active_window, tools);
        if (!metadata)
            return;

        if (metadata->get_type() != WindowType::tiled)
        {
            mir::log_warning("Cannot focus prev when a tiling window is not selected");
            return;
        }

        auto node = metadata->get_tiling_node();
        auto parent = node->get_parent().lock();
        auto index = parent->get_index_of_node(node);
        if (index != parent->num_nodes() - 1)
        {
            auto node_to_select = parent->get_nth_window(index + 1);
            active_output->select_window(node_to_select->get_window());
        }
    }
    else if (arg == "floating")
        mir::log_warning("'focus floating' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "tiling")
        mir::log_warning("'focus tiling' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "mode_toggle")
        mir::log_warning("'focus mode_toggle' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "output")
        mir::log_warning("'focus output' is not supported, see https://github.com/canonical/mir/issues/3357"); // TODO
}