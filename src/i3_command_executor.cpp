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
#include "auto_restarting_launcher.h"
#include "direction.h"
#include "i3_command.h"
#include "leaf_container.h"
#include "parent_container.h"
#include "policy.h"
#include "window_helpers.h"
#include "window_controller.h"

#define MIR_LOG_COMPONENT "miracle"
#include <mir/log.h>
#include <miral/application_info.h>

using namespace miracle;

I3CommandExecutor::I3CommandExecutor(
    miracle::Policy& policy,
    WorkspaceManager& workspace_manager,
    miral::WindowManagerTools const& tools,
    AutoRestartingLauncher& launcher,
    WindowController& window_controller) :
    policy { policy },
    workspace_manager { workspace_manager },
    tools { tools },
    launcher { launcher },
    window_controller { window_controller }
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
        case I3CommandType::sticky:
            process_sticky(command, command_list);
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
            if (command_list.meets_criteria(window, window_controller))
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

    StartupApp app { command.arguments[arg_index], false, no_startup_id };
    launcher.launch(app);
}

void I3CommandExecutor::process_split(miracle::I3Command const& command, miracle::I3ScopedCommandList const& command_list)
{
    if (command.arguments.empty())
    {
        mir::log_warning("process_split: no arguments were supplied");
        return;
    }

    if (command.arguments.front() == "vertical")
    {
        policy.try_request_vertical();
    }
    else if (command.arguments.front() == "horizontal")
    {
        policy.try_request_horizontal();
    }
    else if (command.arguments.front() == "toggle")
    {
        policy.try_toggle_layout();
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
        auto container = window_controller.get_container(window);
        if (container)
            workspace_manager.request_focus(container->get_workspace()->get_workspace());
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

        auto container = window_controller.get_container(active_window);
        if (!container)
            return;

        if (container->get_type() != ContainerType::tiled)
        {
            mir::log_warning("Cannot focus prev when a tiling window is not selected");
            return;
        }

        if (auto parent = Container::as_parent(container->get_parent().lock()))
        {
            auto index = parent->get_index_of_node(container);
            if (index != 0)
            {
                auto node_to_select = parent->get_nth_window(index - 1);
                active_output->select_window(node_to_select->window().value());
            }
        }
    }
    else if (arg == "next")
    {
        auto active_window = tools.active_window();
        if (!active_window)
            return;

        auto container = window_controller.get_container(active_window);
        if (!container)
            return;

        if (container->get_type() != ContainerType::tiled)
        {
            mir::log_warning("Cannot focus prev when a tiling window is not selected");
            return;
        }

        if (auto parent = Container::as_parent(container->get_parent().lock()))
        {
            auto index = parent->get_index_of_node(container);
            if (index != parent->num_nodes() - 1)
            {
                auto node_to_select = parent->get_nth_window(index + 1);
                active_output->select_window(node_to_select->window().value());
            }
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
bool parse_move_distance(std::vector<std::string> const& arguments, int& index, int total_size, int& out)
{
    auto size = arguments.size() - index;
    if (size <= 1)
        return false;

    try
    {
        out = std::stoi(arguments[index]);
        if (size == 2)
        {
            // We default to assuming the value is in pixels
            if (arguments[index + 1] == "ppt")
            {
                float ppt = static_cast<float>(out) / 100.f;
                out = (float)total_size * ppt;
            }
        }

        return true;
    }
    catch (std::invalid_argument const& e)
    {
        mir::log_error("Invalid argument: %s", arguments[index].c_str());
        return false;
    }
}
}

void I3CommandExecutor::process_move(I3Command const& command, I3ScopedCommandList const& command_list)
{
    auto active_output = policy.get_active_output();
    if (!active_output)
    {
        mir::log_warning("process_move: output is not set");
        return;
    }

    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        mir::log_warning("process_move: move command expects arguments");
        return;
    }

    int index = 0;
    auto const& arg0 = command.arguments[index++];
    Direction direction = Direction::MAX;
    int total_size = 0;
    if (arg0 == "left")
    {
        direction = Direction::left;
        total_size = active_output->get_area().size.width.as_int();
    }
    else if (arg0 == "right")
    {
        direction = Direction::right;
        total_size = active_output->get_area().size.width.as_int();
    }
    else if (arg0 == "up")
    {
        direction = Direction::up;
        total_size = active_output->get_area().size.height.as_int();
    }
    else if (arg0 == "down")
    {
        direction = Direction::down;
        total_size = active_output->get_area().size.height.as_int();
    }
    else if (arg0 == "position")
    {
        if (command.arguments.size() < 2)
        {
            mir::log_error("process_move: move position expected a third argument");
            return;
        }

        auto const& arg1 = command.arguments[index++];
        if (arg1 == "center")
        {
            auto active_window = policy.get_state().active_window;
            auto area = active_output->get_area();
            float x = (float)area.size.width.as_int() / 2.f - (float)active_window.size().width.as_int() / 2.f;
            float y = (float)area.size.height.as_int() / 2.f - (float)active_window.size().height.as_int() / 2.f;
            active_output->move_active_window_to((int)x, (int)y);
        }
        else if (arg1 == "mouse")
        {
            auto const& position = policy.get_cursor_position();
            active_output->move_active_window_to((int)position.x.as_int(), (int)position.y.as_int());
        }
        else
        {
            int move_distance_x;
            int move_distance_y;

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_x))
            {
                mir::log_error("process_move: move position <x> <y>: unable to parse x");
                return;
            }

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_y))
            {
                mir::log_error("process_move: move position <x> <y>: unable to parse y");
                return;
            }

            active_output->move_active_window_to(move_distance_x, move_distance_y);
        }
        return;
    }
    else if (arg0 == "absolute")
    {
        auto const& arg1 = command.arguments[index++];
        auto const& arg2 = command.arguments[index++];
        if (arg1 != "position")
        {
            mir::log_error("process_move: move [absolute] ... expected 'position' as the third argument");
            return;
        }

        if (arg2 != "center")
        {
            mir::log_error("process_move: move absolute position ... expected 'center' as the third argument");
            return;
        }

        float x = 0, y = 0;
        for (auto const& output : policy.get_output_list())
        {
            auto area = output->get_area();
            float end_x = (float)area.size.width.as_int() + (float)area.top_left.x.as_int();
            float end_y = (float)area.size.height.as_int() + (float)area.top_left.y.as_int();
            if (end_x > x)
                x = end_x;
            if (end_y > y)
                y = end_y;
        }

        auto active_window = policy.get_state().active_window;
        float x_pos = x / 2.f - (float)active_window.size().width.as_int() / 2.f;
        float y_pos = y / 2.f - (float)active_window.size().height.as_int() / 2.f;
        active_output->move_active_window_to((int)x_pos, (int)y_pos);
        return;
    }

    if (direction < Direction::MAX)
    {
        int move_distance;
        if (parse_move_distance(command.arguments, index, total_size, move_distance))
            active_output->move_active_window_by_amount(direction, move_distance);
        else
            active_output->move_active_window(direction);
    }
}

void I3CommandExecutor::process_sticky(I3Command const& command, I3ScopedCommandList const& command_list)
{
    auto active_output = policy.get_active_output();
    if (!active_output)
    {
        mir::log_warning("process_sticky: output is not set");
        return;
    }

    if (command.arguments.empty())
    {
        mir::log_warning("process_sticky: expects arguments");
        return;
    }

    auto const& arg0 = command.arguments[0];
    if (arg0 == "enable")
        active_output->set_is_pinned(true);
    else if (arg0 == "disable")
        active_output->set_is_pinned(false);
    else if (arg0 == "toggle")
        active_output->toggle_pinned_to_workspace();
    else
        mir::log_warning("process_sticky: unknown arguments: %s", arg0.c_str());
}