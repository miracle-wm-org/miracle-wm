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

#define MIR_LOG_COMPONENT "miracle"

#include "ipc_command_executor.h"
#include "auto_restarting_launcher.h"
#include "command_controller.h"
#include "direction.h"
#include "ipc_command.h"
#include "leaf_container.h"
#include "output_manager.h"
#include "parent_container.h"
#include "utility_general.h"
#include "window_controller.h"

#include <format>
#include <mir/log.h>
#include <miral/application_info.h>

using namespace miracle;

namespace
{
class ArgumentsIndexer
{
public:
    explicit ArgumentsIndexer(IpcCommand const& command) :
        command { command }
    {
    }

    bool next()
    {
        index++;
        return index < command.arguments.size();
    }

    bool prev()
    {
        index--;
        return index < command.arguments.size();
    }

    [[nodiscard]] std::string const& current() const
    {
        return command.arguments[index];
    }

    bool parse_move_distance(int available_area, int& out)
    {
        if (!next())
            return false;

        try
        {
            out = std::stoi(current());
            if (next())
            {
                // We default to assuming the value is in pixels
                if (current() == "ppt")
                {
                    float ppt = static_cast<float>(out) / 100.f;
                    out = static_cast<float>(available_area) * ppt;
                    return true;
                }
                else if (current() == "px")
                {
                    return true;
                }
            }

            // The 'next' item wasn't ppt or px, so let's pop out of it.
            prev();
            return true;
        }
        catch (std::invalid_argument const& e)
        {
            mir::log_error("Invalid argument: %s", command.arguments[index].c_str());
            return false;
        }
    }

protected:
    IpcCommand const& command;
    size_t index = 0;
};
}

IpcCommandExecutor::IpcCommandExecutor(
    std::shared_ptr<CommandController> const& policy,
    std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<CompositorState> const& state,
    AutoRestartingLauncher& launcher,
    std::shared_ptr<WindowController> const& window_controller) :
    policy { policy },
    output_manager { output_manager },
    state { state },
    launcher { launcher },
    window_controller { window_controller }
{
}

std::vector<IpcValidationResult> IpcCommandExecutor::process(IpcParseResult const& command_list)
{
    std::vector<IpcValidationResult> result;
    for (auto const& command : command_list.commands)
    {
        switch (command.type)
        {
        case IpcCommandType::exec:
            result.push_back(process_exec(command, command_list));
            break;
        case IpcCommandType::split:
            result.push_back(process_split(command, command_list));
            break;
        case IpcCommandType::focus:
            result.push_back(process_focus(command, command_list));
            break;
        case IpcCommandType::move:
            result.push_back(process_move(command, command_list));
            break;
        case IpcCommandType::sticky:
            result.push_back(process_sticky(command, command_list));
            break;
        case IpcCommandType::exit:
            policy->quit();
            result.push_back(IpcValidationResult::create_success());
            break;
        case IpcCommandType::input:
            result.push_back(process_input(command, command_list));
            break;
        case IpcCommandType::workspace:
            result.push_back(process_workspace(command, command_list));
            break;
        case IpcCommandType::layout:
            result.push_back(process_layout(command, command_list));
            break;
        case IpcCommandType::scratchpad:
            result.push_back(process_scratchpad(command, command_list));
            break;
        case IpcCommandType::resize:
            result.push_back(process_resize(command, command_list));
            break;
        case IpcCommandType::reload:
            result.push_back(process_reload(command, command_list));
            break;
        default:
            result.push_back(IpcValidationResult::create_failure(std::format("Unsupported command type: {}", command.raw_command), true));
            break;
        }
    }

    return result;
}

IpcValidationResult IpcCommandExecutor::process_exec(IpcCommand const& command, IpcParseResult const&)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("No arguments were supplied", false);

    bool no_startup_id = false;
    if (!command.options.empty() && command.options[0] == "--no-startup-id")
        no_startup_id = true;

    if (command.arguments.empty())
        return IpcValidationResult::create_failure("Argument does not have a command to run", false);

    std::string exec_cmd;
    for (auto const& arg : command.arguments)
    {
        exec_cmd += arg + " ";
    }

    StartupApp const app { exec_cmd, false, no_startup_id };
    launcher.launch(app);
    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_split(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("No arguments were supplied", false);

    if (command.arguments.front() == "vertical")
    {
        policy->try_request_vertical(command_list.scope);
    }
    else if (command.arguments.front() == "horizontal")
    {
        policy->try_request_horizontal(command_list.scope);
    }
    else if (command.arguments.front() == "toggle")
    {
        policy->try_toggle_layout(false, command_list.scope);
    }
    else
    {
        return IpcValidationResult::create_failure(std::format("Unknown argument {}", command.arguments.front().c_str()), false);
    }

    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_focus(IpcCommand const& command, IpcParseResult const& command_list)
{
    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        if (command_list.scope.empty())
            return IpcValidationResult::create_failure("Focus command expected scope but none was provided", false);

        policy->try_select(command_list.scope);
        return IpcValidationResult::create_success();
    }

    auto const& arg = command.arguments.front();
    if (arg == "workspace")
    {
        if (command_list.scope.empty())
            return IpcValidationResult::create_failure("Focus 'workspace' command expected scope but none was provided", false);

        policy->select_workspace_with_scope(command_list.scope);
    }
    else if (arg == "left")
        policy->try_select(Direction::left, command_list.scope);
    else if (arg == "right")
        policy->try_select(Direction::right, command_list.scope);
    else if (arg == "up")
        policy->try_select(Direction::up, command_list.scope);
    else if (arg == "down")
        policy->try_select(Direction::down, command_list.scope);
    else if (arg == "parent")
        policy->try_select_parent(command_list.scope);
    else if (arg == "child")
        policy->try_select_child(command_list.scope);
    else if (arg == "prev")
    {
        // TODO: Move this to the policy because this is racy
        auto const container = state->focused_container();
        if (!container)
            return IpcValidationResult::create_failure("Active container does not exist", false);

        if (container->get_type() != ContainerType::leaf)
            return IpcValidationResult::create_failure("Cannot focus prev when a tiling window is not selected", false);

        if (auto const parent = Container::as_parent(container->get_parent().lock()))
        {
            auto const index = parent->get_index_of_node(container).value();
            if (index != 0)
            {
                auto const node_to_select = parent->get_nth_window(index - 1);
                window_controller->select_active_window(node_to_select->window().value());
            }
        }
    }
    else if (arg == "next")
    {
        // TODO: Move this to the policy because this is racy
        auto const container = state->focused_container();
        if (!container)
            return IpcValidationResult::create_failure("No container is selected", false);

        if (container->get_type() != ContainerType::leaf)
            return IpcValidationResult::create_failure("Cannot focus prev when a tiling window is not selected", false);

        if (auto const parent = Container::as_parent(container->get_parent().lock()))
        {
            auto const index = parent->get_index_of_node(container).value();
            if (index != parent->num_nodes() - 1)
            {
                auto node_to_select = parent->get_nth_window(index + 1);
                window_controller->select_active_window(node_to_select->window().value());
            }
        }
    }
    else if (arg == "floating")
        policy->try_select_floating(command_list.scope);
    else if (arg == "tiling")
        policy->try_select_tiling(command_list.scope);
    else if (arg == "mode_toggle")
        policy->try_select_toggle(command_list.scope);
    else if (arg == "output")
    {
        if (command.arguments.size() < 2)
            return IpcValidationResult::create_failure("'focus output' must have more than two arguments", true);

        auto const& arg1 = command.arguments[1];
        if (arg1 == "next")
            policy->try_select_next_output();
        else if (arg1 == "prev")
            policy->try_select_prev_output();
        else if (arg1 == "left")
            policy->try_select_output(Direction::left);
        else if (arg1 == "right")
            policy->try_select_output(Direction::right);
        else if (arg1 == "up")
            policy->try_select_output(Direction::up);
        else if (arg1 == "down")
            policy->try_select_output(Direction::down);
        else
        {
            auto names = std::vector<std::string>(command.arguments.begin() + 1, command.arguments.end());
            policy->try_select_output(names);
        }
    }

    return IpcValidationResult::create_success();
}

namespace
{
bool parse_move_distance(std::vector<std::string> const& arguments, int& index, int total_size, int& out)
{
    auto const size = arguments.size() - index;
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
                float const ppt = static_cast<float>(out) / 100.f;
                out = static_cast<float>(total_size) * ppt;
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

IpcValidationResult IpcCommandExecutor::process_move(IpcCommand const& command, IpcParseResult const& command_list)
{
    auto const& active_output = output_manager->focused();
    if (!active_output)
        return IpcValidationResult::create_failure("process_move: output is not set", false);

    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("process_move: move command expects arguments", false);

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

    if (direction < Direction::MAX)
    {
        int move_distance;
        if (parse_move_distance(command.arguments, index, total_size, move_distance))
            policy->try_move_by(direction, move_distance, command_list.scope);
        else
            policy->try_move(direction, command_list.scope);
        return IpcValidationResult::create_success();
    }

    if (arg0 == "position")
    {
        if (command.arguments.size() < 2)
            return IpcValidationResult::create_failure("move position expected a third argument", true);

        auto const& arg1 = command.arguments[index++];
        if (arg1 == "center")
        {
            auto const active = state->focused_container().get();
            auto const area = active_output->get_area();
            float const x = static_cast<float>(area.size.width.as_int()) / 2.f - static_cast<float>(active->get_visible_area().size.width.as_int()) / 2.f;
            float const y = static_cast<float>(area.size.height.as_int()) / 2.f - static_cast<float>(active->get_visible_area().size.height.as_int()) / 2.f;
            policy->try_move_to(static_cast<int>(x), static_cast<int>(y), command_list.scope);
        }
        else if (arg1 == "mouse")
        {
            auto const& position = state->cursor_position;
            policy->try_move_to((int)position.x.as_int(), (int)position.y.as_int(), command_list.scope);
        }
        else
        {
            int move_distance_x;
            int move_distance_y;

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_x))
                return IpcValidationResult::create_failure("move position <x> <y>: unable to parse x", true);

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_y))
                return IpcValidationResult::create_failure("move position <x> <y>: unable to parse y", true);

            policy->try_move_to(move_distance_x, move_distance_y, command_list.scope);
        }

        return IpcValidationResult::create_success();
    }
    else if (arg0 == "absolute")
    {
        auto const& arg1 = command.arguments[index++];
        auto const& arg2 = command.arguments[index++];
        if (arg1 != "position")
            return IpcValidationResult::create_failure("move [absolute] ... expected 'position' as the third argument", true);

        if (arg2 != "center")
            return IpcValidationResult::create_failure("move absolute position ... expected 'center' as the third argument", true);

        float x = 0, y = 0;
        for (auto const& output : output_manager->outputs())
        {
            auto area = output->get_area();
            float const end_x = static_cast<float>(area.size.width.as_int() + area.top_left.x.as_int());
            float const end_y = static_cast<float>(area.size.height.as_int() + area.top_left.y.as_int());
            if (end_x > x)
                x = end_x;
            if (end_y > y)
                y = end_y;
        }

        auto const active = state->focused_container();
        float const x_pos = x / 2.f - static_cast<float>(active->get_visible_area().size.width.as_int()) / 2.f;
        float const y_pos = y / 2.f - static_cast<float>(active->get_visible_area().size.height.as_int()) / 2.f;
        policy->try_move_to(static_cast<int>(x_pos), static_cast<int>(y_pos), command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "window" || arg0 == "container")
    {
        auto const back_and_forth = std::find(command.options.begin(), command.options.end(), "--no-auto-back-and-forth") == command.options.end();
        auto const& arg1 = command.arguments[index++];
        if (arg1 != "to")
            return IpcValidationResult::create_failure("Expected 'to' after 'move window/container ...'", true);

        auto const& arg2 = command.arguments[index++];
        if (arg2 == "workspace")
        {
            if (command.arguments.size() <= 3)
                return IpcValidationResult::create_failure("Expected another argument after 'move container/window to output...'", true);

            auto const& arg3 = command.arguments[index++];
            int number = -1;
            if (try_get_number(arg3, number))
            {
                // TODO: Do we need to care about the name here?
                policy->move_active_to_workspace(number, back_and_forth);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "next")
            {
                policy->move_active_to_next_workspace();
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "prev")
            {
                policy->move_active_to_prev_workspace();
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "current")
            {
                // TODO: Support window selection
            }
            else if (arg3 == "back_and_forth")
            {
                policy->move_active_to_back_and_forth();
                return IpcValidationResult::create_success();
            }
            else
            {
                policy->move_active_to_workspace_named(arg3, back_and_forth);
                return IpcValidationResult::create_success();
            }
        }
        else if (arg2 == "output")
        {
            if (command.arguments.size() <= 3)
                return IpcValidationResult::create_failure("Expected another argument after 'move container/window to output...'", true);

            auto const& arg3 = command.arguments[index++];
            if (arg3 == "left")
                policy->try_move_active_to_output(Direction::left);
            else if (arg3 == "right")
                policy->try_move_active_to_output(Direction::right);
            else if (arg3 == "down")
                policy->try_move_active_to_output(Direction::down);
            else if (arg3 == "up")
                policy->try_move_active_to_output(Direction::up);
            else if (arg3 == "current")
                policy->try_move_active_to_current();
            else if (arg3 == "primary")
                policy->try_move_active_to_primary();
            else if (arg3 == "nonprimary")
                policy->try_move_active_to_nonprimary();
            else if (arg3 == "next")
                policy->try_move_active_to_next();
            else
            {
                auto names = std::vector<std::string>(command.arguments.begin() + index - 1, command.arguments.end());
                policy->try_move_active(names);
            }
            return IpcValidationResult::create_success();
        }

        return IpcValidationResult::create_failure("Expected workspace/output after 'move container/window to ...'", true);
    }
    else if (arg0 == "scratchpad")
    {
        policy->move_to_scratchpad();
        return IpcValidationResult::create_success();
    }

    return IpcValidationResult::create_failure("Expected left/right/up/down/position/absolute/window/container/scratchpad after 'move ...'", true);
}

IpcValidationResult IpcCommandExecutor::process_sticky(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("'sticky' expects arguments", true);

    auto const& arg0 = command.arguments[0];
    if (arg0 == "enable")
        policy->set_is_pinned(true, {});
    else if (arg0 == "disable")
        policy->set_is_pinned(false, {});
    else if (arg0 == "toggle")
        policy->toggle_pinned_to_workspace({});
    else
        return IpcValidationResult::create_failure("Expected enable/disable/toggle after 'sticky'", true);

    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_input(IpcCommand const& command, IpcParseResult const& command_list)
{
    // Payloads appear in the following format:
    //    [type:X, xkb_Y, Z]
    // where X is something like "keyboard", Y is the variable that we want to change
    // and Z is the value of that variable. Z may not be included at all, in which
    // case the variable is set to the default.
    if (command.arguments.size() < 2)
        return IpcValidationResult::create_failure("Expected at least 2 arguments for 'input'", true);

    constexpr char* const TYPE_PREFIX = "type:";
    const size_t TYPE_PREFIX_LEN = strlen(TYPE_PREFIX);
    std::string_view type_str = command.arguments[0];
    if (!type_str.starts_with("type:"))
        return IpcValidationResult::create_failure(std::format("'type' string is misformatted: {}", command.arguments[0].c_str()), true);

    std::string_view const type = type_str.substr(TYPE_PREFIX_LEN);
    assert(type == "keyboard");

    std::string_view const xkb_str = command.arguments[1];
    constexpr char* const XKB_PREFIX = "xkb_";
    const size_t XKB_PREFIX_LEN = strlen(XKB_PREFIX);
    if (!xkb_str.starts_with(XKB_PREFIX))
        return IpcValidationResult::create_failure(std::format("'xkb' string is misformatted: {}", command.arguments[1].c_str()), true);

    std::string_view const xkb_variable_name = xkb_str.substr(XKB_PREFIX_LEN);
    if (xkb_variable_name != "model"
        && xkb_variable_name != "layout"
        && xkb_variable_name != "variant"
        && xkb_variable_name != "options")
    {
        return IpcValidationResult::create_failure("Expected xkb variable name to be xkb_model, xkb_layout, xkb_variant, or xkb_options", true);
    }

    mir::log_info("Processing input from locale1: type=%s, xkb_variable=%s", type.data(), xkb_variable_name.data());

    // TODO: This is where we need to process the request
    if (command.arguments.size() == 3)
    {
    }
    else if (command.arguments.size() < 3)
    {
        // TODO: Set to the default
    }
    else
        return IpcValidationResult::create_failure("Received > 3 arguments for 'input' command but only <= 3 are expected", true);

    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_workspace(IpcCommand const& command, IpcParseResult const&)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("Expected arguments for 'workspace' command", true);

    std::string const& arg0 = command.arguments[0];
    if (arg0 == "next")
    {
        policy->next_workspace();
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "prev")
    {
        policy->prev_workspace();
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "next_on_output")
    {
        if (auto const& output = output_manager->focused())
        {
            policy->next_workspace_on_output(*output);
            return IpcValidationResult::create_success();
        }
        else
            return IpcValidationResult::create_failure("'workspace next_on_output' has no output to go next on", false);
    }
    else if (arg0 == "prev_on_output")
    {
        if (auto const& output = output_manager->focused())
        {
            policy->prev_workspace_on_output(*output);
            return IpcValidationResult::create_success();
        }
        else
            return IpcValidationResult::create_failure("'workspace prev_on_output' has no output to go prev on", false);
    }
    else if (arg0 == "back_and_forth")
    {
        policy->back_and_forth_workspace();
        return IpcValidationResult::create_success();
    }

    std::string const* arg1 = &arg0;
    auto const back_and_forth = std::ranges::find(command.options, "--no-auto-back-and-forth") == command.options.end();

    int number = -1;
    if (try_get_number(*arg1, number))
    {
        // Check if we just have "workspace number"
        if (command.arguments.size() < 3)
        {
            policy->select_workspace(number, back_and_forth);
            return IpcValidationResult::create_success();
        }

        // We have "workspace number <name>"
        arg1 = &command.arguments[2];
        policy->select_workspace(*arg1, back_and_forth);
        return IpcValidationResult::create_success();
    }
    else
    {
        // We have "workspace <name>"
        policy->select_workspace(*arg1, back_and_forth);
        return IpcValidationResult::create_success();
    }
}

IpcValidationResult IpcCommandExecutor::process_layout(IpcCommand const& command, IpcParseResult const& command_list)
{
    // https://i3wm.org/docs/userguide.html#manipulating_layout
    std::string const& arg0 = command.arguments[0];
    if (arg0 == "default")
    {
        policy->set_layout_default({});
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "tabbed")
    {
        policy->set_layout(LayoutScheme::tabbing, {});
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "stacking")
    {
        policy->set_layout(LayoutScheme::stacking, {});
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "splitv")
    {
        policy->set_layout(LayoutScheme::vertical, {});
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "splith")
    {
        policy->set_layout(LayoutScheme::horizontal, {});
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "toggle")
    {
        if (command.arguments.size() == 1)
            return IpcValidationResult::create_failure("Expected argument after 'layout toggle ...'", true);

        if (command.arguments.size() == 2)
        {
            auto const& arg1 = command.arguments[1];
            if (arg1 == "split")
            {
                policy->try_toggle_layout(false, command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg1 == "all")
            {
                policy->try_toggle_layout(true, command_list.scope);
                return IpcValidationResult::create_success();
            }

            return IpcValidationResult::create_failure("Expected split/all after 'layout toggle X'", true);
        }

        auto const container = state->focused_container();
        if (!container)
            return IpcValidationResult::create_failure("Container unavailable during 'layout'", false);

        auto const current_type = container->get_layout();
        size_t index = 0;
        for (size_t i = 1; i < command.arguments.size(); i++)
        {
            auto const& argn = command.arguments[i];
            if (argn == "split")
            {
                if (current_type == LayoutScheme::horizontal || current_type == LayoutScheme::vertical)
                {
                    index = i;
                    break;
                }
            }
            else if (argn == "tabbed")
            {
                if (current_type == LayoutScheme::tabbing)
                {
                    index = i;
                    break;
                }
            }
            else if (argn == "stacking")
            {
                if (current_type == LayoutScheme::stacking)
                {
                    index = i;
                    break;
                }
            }
            else if (argn == "splitv")
            {
                if (current_type == LayoutScheme::vertical)
                {
                    index = i;
                    break;
                }
            }
            else if (argn == "splith")
            {
                if (current_type == LayoutScheme::horizontal)
                {
                    index = i;
                    break;
                }
            }
        }

        index++;
        if (index == command.arguments.size())
            index = 1;

        auto const& target = command.arguments[index];
        if (target == "split")
        {
            policy->try_toggle_layout(false, command_list.scope);
            return IpcValidationResult::create_success();
        }
        else if (target == "tabbed")
        {
            policy->set_layout(LayoutScheme::tabbing, {});
            return IpcValidationResult::create_success();
        }
        else if (target == "stacking")
        {
            policy->set_layout(LayoutScheme::stacking, {});
            return IpcValidationResult::create_success();
        }
        else if (target == "splitv")
        {
            policy->set_layout(LayoutScheme::vertical, {});
            return IpcValidationResult::create_success();
        }
        else if (target == "splith")
        {
            policy->set_layout(LayoutScheme::horizontal, {});
            return IpcValidationResult::create_success();
        }
        else
        {
            return IpcValidationResult::create_failure(std::format("Unknown argument to 'layout' command, {}", target), true);
        }
    }
    else
    {
        return IpcValidationResult::create_failure("Expected default/tabbed/stacking/splitv/splith/toggle after 'layout'", true);
    }
}

IpcValidationResult IpcCommandExecutor::process_scratchpad(IpcCommand const& command, IpcParseResult const&)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("No arguments provided to 'scratchpad' command", true);

    std::string const& arg0 = command.arguments[0];
    if (arg0 != "show")
        return IpcValidationResult::create_failure("Expected 'show' after 'scratchpad'", true);

    policy->show_scratchpad();
    return IpcValidationResult::create_success();
}

namespace
{
struct ResizeAdjust
{
    bool success = true;
    std::string error;
    Direction direction = Direction::MAX;
    int first = 0;
    int second = 0;
};

ResizeAdjust parse_resize(std::shared_ptr<CompositorState> const& state, ArgumentsIndexer& indexer, int multiplier)
{
    if (!indexer.next())
        return { .success = false, .error = "process_resize: expected argument after 'resize grow'" };

    auto const& container = state->focused_container();
    if (!container)
        return { .success = false, .error = "No container is selcted" };

    ResizeAdjust result;
    if (indexer.current() == "width" || indexer.current() == "horizontal")
    {
        result.direction = Direction::right;
    }
    else if (indexer.current() == "height" || indexer.current() == "vertical")
    {
        result.direction = Direction::down;
    }
    else if (indexer.current() == "up")
    {
        result.direction = Direction::up;
    }
    else if (indexer.current() == "down")
    {
        result.direction = Direction::down;
    }
    else if (indexer.current() == "left")
    {
        result.direction = Direction::left;
    }
    else if (indexer.current() == "right")
    {
        result.direction = Direction::right;
    }
    else
    {
        return { .success = false, .error = std::format("Unknown direction value: {}", indexer.current().c_str()) };
    }

    int available_space = 0;
    switch (result.direction)
    {
    case Direction::up:
    case Direction::down:
        available_space = container->get_output()->get_area().size.height.as_value();
        break;
    default:
        available_space = container->get_output()->get_area().size.width.as_value();
        break;
    }

    int first = 0;
    if (!indexer.parse_move_distance(available_space, first))
        return { .success = false, .error = "cannot parse the first value" };

    if (indexer.next())
    {
        if (indexer.current() != "or")
            return { .success = false, .error = "expected 'or' after first value" };
    }

    int second = 0;
    indexer.parse_move_distance(available_space, second);
    result.first = first * multiplier;
    result.second = second * multiplier;
    return result;
}

struct SetResizeResult
{
    bool success = true;
    std::string error;
    std::optional<int> width;
    std::optional<int> height;
};

SetResizeResult parse_set_resize(std::shared_ptr<CompositorState> const& state, ArgumentsIndexer& indexer)
{
    auto const& container = state->focused_container();
    if (!container)
        return { .success = false, .error = "Container is not selected" };

    SetResizeResult result;
    int width = 0, height = 0;
    if (!indexer.parse_move_distance(container->get_output()->get_area().size.width.as_value(), width))
        return { .success = false, .error = "invalid width" };

    if (!indexer.parse_move_distance(container->get_output()->get_area().size.height.as_value(), height))
        return { .success = false, .error = "invalid height" };

    if (width != 0)
        result.width = width;
    if (height != 0)
        result.height = height;

    return result;
}
}

IpcValidationResult IpcCommandExecutor::process_resize(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("Expected arguments to be provided to 'resize'", true);

    ArgumentsIndexer indexer(command);
    auto const& arg0 = indexer.current();
    if (arg0 == "grow")
    {
        auto const adjust = parse_resize(state, indexer, 1);
        if (!adjust.success)
            return IpcValidationResult::create_failure(adjust.error, true);

        policy->try_resize(adjust.direction, adjust.first, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "shrink")
    {
        auto const adjust = parse_resize(state, indexer, -1);
        if (!adjust.success)
            return IpcValidationResult::create_failure(adjust.error, true);

        policy->try_resize(adjust.direction, adjust.first, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "set")
    {
        auto const result = parse_set_resize(state, indexer);
        if (!result.success)
            return IpcValidationResult::create_failure(result.error, true);

        policy->try_set_size(result.width, result.height, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else
        return IpcValidationResult::create_failure(std::format("Encountered unexpected argument during 'resize': {}", arg0.c_str()), true);
}

IpcValidationResult IpcCommandExecutor::process_reload(IpcCommand const& command, IpcParseResult const&)
{
    if (!command.arguments.empty())
        return IpcValidationResult::create_failure("'reload' command expects no arguments", true);

    policy->reload_config();
    return IpcValidationResult::create_success();
}
