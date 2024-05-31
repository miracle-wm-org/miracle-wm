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
#include "direction.h"
#include "i3_command.h"
#include "leaf_node.h"
#include "parent_node.h"
#include "policy.h"
#include "window_helpers.h"
#include "auto_restarting_launcher.h"

#define MIR_LOG_COMPONENT "miracle"
#include <mir/log.h>
#include <miral/application_info.h>

using namespace miracle;

I3CommandExecutor::I3CommandExecutor(
    miracle::Policy& policy,
    WorkspaceManager& workspace_manager,
    miral::WindowManagerTools const& tools,
    AutoRestartingLauncher& launcher) :
    policy { policy },
    workspace_manager { workspace_manager },
    tools { tools },
    launcher{ launcher }
{
}

void I3CommandExecutor::process(miracle::I3ScopedCommandList const& command_list)
{
    for (auto const& command : command_list.commands)
    {
        switch (command.type)
        {
        case I3CommandType::exec:
            process_exec(command, command_list);
            break;
        case I3CommandType::split:
            process_split(command, command_list);
            break;
        case I3CommandType::focus:
            process_focus(command, command_list);
            break;
        case I3CommandType::move:
            process_move(command, command_list);
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

void I3CommandExecutor::process_exec(miracle::I3Command const& command, miracle::I3ScopedCommandList const& command_list)
{
    if (command.arguments.empty())
    {
        mir::log_warning("process_exec: no arguments were supplied");
        return;
    }

    size_t arg_index = 0;
    bool no_startup_id = false;
    if (command.arguments[arg_index] == "--no-startup-id")
    {
        no_startup_id = true;
        arg_index++;
    }

    if (arg_index >= command.arguments.size())
    {
        mir::log_warning("process_exec: argument does not have a command to run");
        return;
    }

    StartupApp app{ command.arguments[arg_index], false, no_startup_id };
    launcher.launch(app);
}

void I3CommandExecutor::process_split(miracle::I3Command const& command, miracle::I3ScopedCommandList const& command_list)
{
    auto active_output = policy.get_active_output();
    if (!active_output)
    {
        mir::log_warning("process_split: output is null");
        return;
    }

    if (command.arguments.empty())
    {
        mir::log_warning("process_split: no arguments were supplied");
        return;
    }

    if (command.arguments.front() == "vertical")
    {
        active_output->request_vertical();
    }
    else if (command.arguments.front() == "horizontal")
    {
        active_output->request_horizontal();
    }
    else if (command.arguments.front() == "toggle")
    {
        active_output->toggle_layout();
    }
    else
    {
        mir::log_warning("process_split: unknown argument %s", command.arguments.front().c_str());
        return;
    }
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
            workspace_manager.request_focus(metadata->get_workspace()->get_workspace());
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

namespace
{
bool parse_move_distance(std::vector<std::string> const& arguments, int total_size, int& out)
{
    if (arguments.size() <= 1)
        return false;

    try
    {
        out = std::stoi(arguments[1]);
        if (arguments.size() > 2)
        {
            // We default to assuming the value is in pixels
            if (arguments[2] == "ppt")
            {
                float ppt = static_cast<float>(out) / 100.f;
                out = total_size * ppt;
            }
        }

        return true;
    }
    catch (std::invalid_argument const& e)
    {
        mir::log_error("Invalid argument: %s", arguments[1].c_str());
        return false;
    }
}
}

void I3CommandExecutor::process_move(I3Command const& command, I3ScopedCommandList const& command_list)
{
    auto active_output = policy.get_active_output();
    if (!active_output)
    {
        mir::log_warning("Trying to process I3 move command, but output is not set");
        return;
    }
    
    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        mir::log_warning("move command expects arguments");
        return;
    }

    auto const& arg = command.arguments.front();

    Direction direction = Direction::MAX;
    int total_size = 0;
    if (arg == "left")
    {
        direction = Direction::left;
        total_size = active_output->get_area().size.width.as_int();
    }
    else if (arg == "right")
    {
        direction = Direction::right;
        total_size = active_output->get_area().size.width.as_int();
    }
    else if (arg == "up")
    {
        direction = Direction::up;
        total_size = active_output->get_area().size.height.as_int();
    }
    else if (arg == "down")
    {
        direction = Direction::down;
        total_size = active_output->get_area().size.height.as_int();
    }
    else if (arg == "absolute")
    {
        if (command.arguments[2] != "position")
        {
            mir::log_error("move [absolute] ... expected 'position' as the third argument");
            return;
        }

        if (command.arguments[3] != "center")
        {
            mir::log_error("move absolute position ... expected 'center' as the third argument");
            return;
        }

        int x = 0, y = 0;
        for (auto const& output : policy.get_output_list())
        {
            auto area = output->get_area();
            int end_x = area.size.width.as_int() + area.top_left.x.as_int();
            int end_y = area.size.width.as_int() + area.top_left.x.as_int();
            if (end_x > x)
                x = end_x;
            if (end_y > y)
                y = end_y;
        }


    }
    else if (arg == "position")
    {
        if (arg.size() < 3)
        {
            mir::log_error("move position expected a third argument");
            return;
        }

        if (command.arguments[2] == "center")
        {
            active_output->move_active_window_to_center_point()
        }
        else if (command.arguments[2] == "mouse")
        {

        }
        else
        {
            // Parse position X and position Y
        }
    }

    if (direction < Direction::MAX)
    {
        int move_distance;
        if (parse_move_distance(command.arguments, total_size, move_distance))
            active_output->move_active_window_by_amount(direction, move_distance);
        else
            active_output->move_active_window(direction);
    }
}
