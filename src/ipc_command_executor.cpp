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
struct ParseMoveResult
{
    bool const success;
    enum class MoveType
    {
        pixel,
        ppt,
        invalid
    } const move_type;
    float const amount;
};

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

    [[nodiscard]] bool has_current() const
    {
        return index < command.arguments.size();
    }

    [[nodiscard]] std::vector<std::string> current_remaining() const
    {
        return std::vector(command.arguments.begin() + index, command.arguments.end());
    }

    ParseMoveResult parse_move_distance()
    {
        if (!next())
            return ParseMoveResult(false, ParseMoveResult::MoveType::invalid, 0);

        try
        {
            auto move_type = ParseMoveResult::MoveType::pixel;
            float amount = std::stoi(current());
            bool has_type_label = false;
            if (next())
            {
                // We default to assuming the value is in pixels
                if (current() == "ppt")
                {
                    amount = amount / 100.f;
                    move_type = ParseMoveResult::MoveType::ppt;
                    has_type_label = true;
                }
                else if (current() == "px")
                {
                    has_type_label = true;
                }
            }

            // The 'next' item wasn't ppt or px, so let's pop out of it.
            if (!has_type_label)
                prev();
            return ParseMoveResult(true, move_type, amount);
        }
        catch (std::invalid_argument const& e)
        {
            mir::log_error("Invalid argument: %s", command.arguments[index].c_str());
            return ParseMoveResult(false, ParseMoveResult::MoveType::invalid, 0);
        }
    }

protected:
    IpcCommand const& command;
    size_t index = 0;
};
}

IpcCommandExecutor::IpcCommandExecutor(
    std::shared_ptr<AbstractCommandController> const& command_controller,
    std::shared_ptr<Launcher> const& launcher) :
    command_controller { command_controller },
    launcher { launcher }
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
            command_controller->quit();
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
        case IpcCommandType::fullscreen:
            result.push_back(process_fullscreen(command, command_list));
            break;
        case IpcCommandType::floating:
            result.push_back(process_floating(command, command_list));
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
        case IpcCommandType::mark:
            result.push_back(process_mark(command, command_list));
            break;
        case IpcCommandType::unmark:
            result.push_back(process_unmark(command, command_list));
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
        return IpcValidationResult::create_failure("No arguments were supplied", true);

    bool no_startup_id = false;
    if (!command.options.empty() && command.options[0] == "--no-startup-id")
        no_startup_id = true;

    std::string exec_cmd;
    for (auto const& arg : command.arguments)
    {
        exec_cmd += arg + " ";
    }

    StartupApp const app { exec_cmd, false, no_startup_id };
    launcher->launch(app);
    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_split(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("No arguments were supplied", true);

    if (command.arguments.front() == "vertical")
    {
        command_controller->try_request_vertical(command_list.scope);
    }
    else if (command.arguments.front() == "horizontal")
    {
        command_controller->try_request_horizontal(command_list.scope);
    }
    else if (command.arguments.front() == "toggle")
    {
        command_controller->try_toggle_layout(false, command_list.scope);
    }
    else
    {
        return IpcValidationResult::create_failure(std::format("Unknown argument {}", command.arguments.front()), true);
    }

    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_focus(IpcCommand const& command, IpcParseResult const& command_list)
{
    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        if (command_list.scope.empty())
            return IpcValidationResult::create_failure("'focus' command expected scope but none was provided", true);

        command_controller->try_select(command_list.scope);
        return IpcValidationResult::create_success();
    }

    auto const& arg = command.arguments.front();
    if (arg == "workspace")
    {
        if (command_list.scope.empty())
            return IpcValidationResult::create_failure("'focus workspace' command expected scope but none was provided", true);

        command_controller->select_workspace_with_scope(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "left")
    {
        command_controller->try_select(Direction::left, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "right")
    {
        command_controller->try_select(Direction::right, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "up")
    {
        command_controller->try_select(Direction::up, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "down")
    {
        command_controller->try_select(Direction::down, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "parent")
    {
        command_controller->try_select_parent(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "child")
    {
        command_controller->try_select_child(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "prev")
    {
        if (!command_controller->try_select_prev(command_list.scope))
            return IpcValidationResult::create_failure("Failed to select previous container", false);

        return IpcValidationResult::create_success();
    }
    else if (arg == "next")
    {
        if (!command_controller->try_select_next(command_list.scope))
            return IpcValidationResult::create_failure("Failed to select next container", false);

        return IpcValidationResult::create_success();
    }
    else if (arg == "floating")
    {
        command_controller->try_select_floating(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "tiling")
    {
        command_controller->try_select_tiling(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "mode_toggle")
    {
        command_controller->try_select_toggle(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg == "output")
    {
        if (command.arguments.size() < 2)
            return IpcValidationResult::create_failure("'focus output' must have more than two arguments", true);

        auto const& arg1 = command.arguments[1];
        if (arg1 == "next")
        {
            command_controller->try_select_next_output();
            return IpcValidationResult::create_success();
        }
        else if (arg1 == "prev")
        {
            command_controller->try_select_prev_output();
            return IpcValidationResult::create_success();
        }
        else if (arg1 == "left")
        {
            command_controller->try_select_output(Direction::left);
            return IpcValidationResult::create_success();
        }
        else if (arg1 == "right")
        {
            command_controller->try_select_output(Direction::right);
            return IpcValidationResult::create_success();
        }
        else if (arg1 == "up")
        {
            command_controller->try_select_output(Direction::up);
            return IpcValidationResult::create_success();
        }
        else if (arg1 == "down")
        {
            command_controller->try_select_output(Direction::down);
            return IpcValidationResult::create_success();
        }
        else
        {
            auto const names = std::vector<std::string>(command.arguments.begin() + 1, command.arguments.end());
            command_controller->try_select_output(names);
            return IpcValidationResult::create_success();
        }
    }
    else
    {
        return IpcValidationResult::create_failure(std::format("Unknown argument to 'focus' command: {}", arg.c_str()), true);
    }
}

IpcValidationResult IpcCommandExecutor::process_move(IpcCommand const& command, IpcParseResult const& command_list)
{
    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("'move' command expects arguments", true);

    ArgumentsIndexer indexer(command);
    auto direction = Direction::MAX;

    if (indexer.current() == "left")
        direction = Direction::left;
    else if (indexer.current() == "right")
        direction = Direction::right;
    else if (indexer.current() == "up")
        direction = Direction::up;
    else if (indexer.current() == "down")
        direction = Direction::down;

    if (direction < Direction::MAX)
    {
        const auto [success, move_type, amount] = indexer.parse_move_distance();
        if (success)
        {
            if (move_type == ParseMoveResult::MoveType::ppt)
                command_controller->try_move_by_ppt(direction, amount, command_list.scope);
            else
                command_controller->try_move_by_pixels(direction, static_cast<int>(amount), command_list.scope);
        }
        else
            command_controller->try_move_by_direction(direction, command_list.scope);
        return IpcValidationResult::create_success();
    }

    if (indexer.current() == "position")
    {
        if (!indexer.next())
            return IpcValidationResult::create_failure("'move position' expected a third argument", true);

        if (indexer.current() == "center")
        {
            command_controller->try_move_to_center_of_active_output(command_list.scope);
            return IpcValidationResult::create_success();
        }
        else if (indexer.current() == "mouse")
        {
            command_controller->try_move_to_cursor(command_list.scope);
            return IpcValidationResult::create_success();
        }
        else
        {
            indexer.prev(); // Moving back by one because [parse_move_distance] jumps next immediately
            auto x_move_distance = indexer.parse_move_distance();
            if (!x_move_distance.success)
                return IpcValidationResult::create_failure("move position <x> <y>: unable to parse x", true);

            auto y_move_distance = indexer.parse_move_distance();
            if (!y_move_distance.success)
                return IpcValidationResult::create_failure("move position <x> <y>: unable to parse y", true);

            command_controller->try_move_to(
                x_move_distance.amount,
                x_move_distance.move_type == ParseMoveResult::MoveType::ppt,
                y_move_distance.amount,
                y_move_distance.move_type == ParseMoveResult::MoveType::ppt,
                command_list.scope);
        }

        return IpcValidationResult::create_success();
    }
    else if (indexer.current() == "absolute")
    {
        if (!indexer.next())
            return IpcValidationResult::create_failure("'move absolute' expected a third argument", true);

        auto const& arg1 = indexer.current();
        if (!indexer.next())
            return IpcValidationResult::create_failure("'move absolute position' expected a fourth argument", true);

        auto const& arg2 = indexer.current();
        if (arg1 != "position")
            return IpcValidationResult::create_failure("'move absolute' ... expected 'position' as the third argument", true);

        if (arg2 != "center")
            return IpcValidationResult::create_failure("'move absolute position' ... expected 'center' as the fourth argument", true);

        command_controller->try_move_to_absolute_center(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (indexer.current() == "window" || indexer.current() == "container")
    {
        if (!indexer.next())
            return IpcValidationResult::create_failure("'move window/container' expected a third argument", true);

        auto const back_and_forth = std::find(command.options.begin(), command.options.end(), "--no-auto-back-and-forth") == command.options.end();
        auto const& arg1 = indexer.current();
        if (arg1 != "to")
            return IpcValidationResult::create_failure("Expected 'to' after 'move window/container ...'", true);

        if (!indexer.next())
            return IpcValidationResult::create_failure("'move window/container to' expected another argument", true);

        auto const& arg2 = indexer.current();
        if (arg2 == "workspace")
        {
            if (!indexer.next())
                return IpcValidationResult::create_failure("Expected another argument after 'move container/window to workspace...'", true);

            auto const& arg3 = indexer.current();
            int number = -1;
            if (try_get_number(arg3, number))
            {
                // TODO: Do we need to care about the name here?
                command_controller->try_move_to_workspace(command_list.scope, number, back_and_forth);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "next")
            {
                command_controller->try_move_to_next_workspace(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "prev")
            {
                command_controller->try_move_to_prev_workspace(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "current")
            {
                command_controller->try_move_to_current_workspace(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "back_and_forth")
            {
                command_controller->try_move_to_back_and_forth(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else
            {
                command_controller->try_move_to_workspace_named(command_list.scope, arg3, back_and_forth);
                return IpcValidationResult::create_success();
            }
        }
        else if (arg2 == "output")
        {
            if (!indexer.next())
                return IpcValidationResult::create_failure("Expected another argument after 'move container/window to output...'", true);

            auto const& arg3 = indexer.current();
            if (arg3 == "left")
            {
                command_controller->try_move_to_output_by_direction(Direction::left, command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "right")
            {
                command_controller->try_move_to_output_by_direction(Direction::right, command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "down")
            {
                command_controller->try_move_to_output_by_direction(Direction::down, command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "up")
            {
                command_controller->try_move_to_output_by_direction(Direction::up, command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "current")
            {
                command_controller->try_move_to_current_output(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "primary")
            {
                command_controller->try_move_to_primary_output(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "nonprimary")
            {
                command_controller->try_move_to_nonprimary_output(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg3 == "next")
            {
                command_controller->try_move_to_next_output(command_list.scope);
                return IpcValidationResult::create_success();
            }
            else
            {
                command_controller->try_move_to_output_by_name_list(indexer.current_remaining(), command_list.scope);
                return IpcValidationResult::create_success();
            }
        }

        return IpcValidationResult::create_failure("Expected workspace/output after 'move container/window to ...'", true);
    }
    else if (indexer.current() == "scratchpad")
    {
        command_controller->try_move_to_scratchpad(command_list.scope);
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
        command_controller->set_is_pinned(true, {});
    else if (arg0 == "disable")
        command_controller->set_is_pinned(false, {});
    else if (arg0 == "toggle")
        command_controller->toggle_pinned_to_workspace({});
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
        command_controller->next_workspace();
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "prev")
    {
        command_controller->prev_workspace();
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "next_on_output")
    {
        if (command_controller->next_workspace_on_output())
            return IpcValidationResult::create_success();
        else
            return IpcValidationResult::create_failure("'workspace next_on_output' has no output to go next on", false);
    }
    else if (arg0 == "prev_on_output")
    {
        if (command_controller->prev_workspace_on_output())
            return IpcValidationResult::create_success();
        else
            return IpcValidationResult::create_failure("'workspace prev_on_output' has no output to go prev on", false);
    }
    else if (arg0 == "back_and_forth")
    {
        command_controller->back_and_forth_workspace();
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
            command_controller->select_workspace(number, back_and_forth);
            return IpcValidationResult::create_success();
        }

        // We have "workspace number <name>"
        arg1 = &command.arguments[2];
        command_controller->select_workspace(*arg1, back_and_forth);
        return IpcValidationResult::create_success();
    }
    else
    {
        // We have "workspace <name>"
        command_controller->select_workspace(*arg1, back_and_forth);
        return IpcValidationResult::create_success();
    }
}

IpcValidationResult IpcCommandExecutor::process_layout(IpcCommand const& command, IpcParseResult const& command_list)
{
    // https://i3wm.org/docs/userguide.html#manipulating_layout
    ArgumentsIndexer indexer(command);
    if (!indexer.has_current())
        return IpcValidationResult::create_failure("No arguments were supplied", true);

    std::string const& arg0 = indexer.current();
    if (arg0 == "default")
    {
        command_controller->set_layout_default(command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "tabbed")
    {
        command_controller->set_layout(LayoutScheme::tabbing, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "stacking")
    {
        command_controller->set_layout(LayoutScheme::stacking, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "splitv")
    {
        command_controller->set_layout(LayoutScheme::vertical, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "splith")
    {
        command_controller->set_layout(LayoutScheme::horizontal, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "toggle")
    {
        if (!indexer.next())
            return IpcValidationResult::create_failure("Expected argument after 'layout toggle ...'", true);

        if (command.arguments.size() == 2)
        {
            auto const& arg1 = indexer.current();
            if (arg1 == "split")
            {
                command_controller->try_toggle_layout(false, command_list.scope);
                return IpcValidationResult::create_success();
            }
            else if (arg1 == "all")
            {
                command_controller->try_toggle_layout(true, command_list.scope);
                return IpcValidationResult::create_success();
            }
        }

        std::vector<LayoutRequestType> request_types;
        do
        {
            auto const& argn = indexer.current();
            if (argn == "split")
                request_types.push_back(LayoutRequestType::split);
            else if (argn == "tabbed")
                request_types.push_back(LayoutRequestType::tabbed);
            else if (argn == "stacking")
                request_types.push_back(LayoutRequestType::stacking);
            else if (argn == "splitv")
                request_types.push_back(LayoutRequestType::splitv);
            else if (argn == "splith")
                request_types.push_back(LayoutRequestType::splith);
            else
                return IpcValidationResult::create_failure(std::format("Unknown layout type in 'layout toggle': {}", argn.c_str()), true);
        } while (indexer.next());

        command_controller->try_cycle_through_request_types(request_types, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else
    {
        return IpcValidationResult::create_failure("Expected default/tabbed/stacking/splitv/splith/toggle after 'layout'", true);
    }
}

IpcValidationResult IpcCommandExecutor::process_fullscreen(IpcCommand const& command, IpcParseResult const& parse_result)
{
    // https://i3wm.org/docs/userguide.html#manipulating_layout
    ArgumentsIndexer indexer(command);
    if (!indexer.has_current())
        return IpcValidationResult::create_failure("No arguments were supplied", true);

    if (indexer.current() != "toggle")
        return IpcValidationResult::create_failure(std::format("Expected 'toggle' after 'fullscreen', but got '{}'", indexer.current()), true);

    command_controller->try_toggle_fullscreen(parse_result.scope);
    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_floating(IpcCommand const& command, IpcParseResult const& parse_result)
{
    // https://i3wm.org/docs/userguide.html#manipulating_layout
    ArgumentsIndexer indexer(command);
    if (!indexer.has_current())
        return IpcValidationResult::create_failure("No arguments were supplied", true);

    if (indexer.current() != "toggle")
        return IpcValidationResult::create_failure(std::format("Expected 'toggle' after 'floating', but got '{}'", indexer.current()), true);

    command_controller->toggle_floating(parse_result.scope);
    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_scratchpad(IpcCommand const& command, IpcParseResult const&)
{
    if (command.arguments.empty())
        return IpcValidationResult::create_failure("No arguments provided to 'scratchpad' command", true);

    std::string const& arg0 = command.arguments[0];
    if (arg0 != "show")
        return IpcValidationResult::create_failure("Expected 'show' after 'scratchpad'", true);

    command_controller->show_scratchpad();
    return IpcValidationResult::create_success();
}

namespace
{
struct ResizeAdjust
{
    bool success = true;
    std::string error;
    Direction direction = Direction::MAX;
    int multiplier = 1;
    std::optional<ParseMoveResult> first;
    std::optional<ParseMoveResult> second;
};

ResizeAdjust parse_resize(ArgumentsIndexer& indexer, int multiplier)
{
    if (!indexer.next())
        return { .success = false, .error = "process_resize: expected argument after 'resize grow'" };

    Direction direction;
    ;
    if (indexer.current() == "width" || indexer.current() == "horizontal")
        direction = Direction::right;
    else if (indexer.current() == "height" || indexer.current() == "vertical")
        direction = Direction::down;
    else if (indexer.current() == "up")
        direction = Direction::up;
    else if (indexer.current() == "down")
        direction = Direction::down;
    else if (indexer.current() == "left")
        direction = Direction::left;
    else if (indexer.current() == "right")
        direction = Direction::right;
    else
        return { .success = false, .error = std::format("Unknown direction value: {}", indexer.current().c_str()) };

    auto const first_move_distance = indexer.parse_move_distance();
    if (!first_move_distance.success)
        return { .success = false, .error = "cannot parse the first value" };

    if (indexer.next())
    {
        if (indexer.current() != "or")
            return { .success = false, .error = "expected 'or' after first value" };
    }

    auto const second_move_distance = indexer.parse_move_distance();
    return ResizeAdjust(true, "", direction, multiplier, first_move_distance, second_move_distance);
}

struct SetResizeResult
{
    bool success = true;
    std::string error;
    ParseMoveResult width;
    ParseMoveResult height;
};

SetResizeResult parse_set_resize(ArgumentsIndexer& indexer)
{
    ParseMoveResult const width_result = indexer.parse_move_distance();
    if (!width_result.success)
        return { .success = false, .error = "invalid width" };

    ParseMoveResult const height_result = indexer.parse_move_distance();
    if (!height_result.success)
        return { .success = false, .error = "invalid height" };

    return SetResizeResult(true, "", width_result, height_result);
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
        auto const adjust = parse_resize(indexer, 1);
        if (!adjust.success)
            return IpcValidationResult::create_failure(adjust.error, true);

        if (!adjust.first.has_value())
            return IpcValidationResult::create_failure("Expected a value after 'grow'", true);

        if (adjust.first->move_type == ParseMoveResult::MoveType::ppt)
            command_controller->try_resize_ppt(adjust.direction, adjust.first->amount, command_list.scope);
        else
            command_controller->try_resize(adjust.direction, adjust.first->amount, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "shrink")
    {
        auto const adjust = parse_resize(indexer, -1);
        if (!adjust.success)
            return IpcValidationResult::create_failure(adjust.error, true);

        if (!adjust.first.has_value())
            return IpcValidationResult::create_failure("Expected a value after 'shrink'", true);

        if (adjust.first->move_type == ParseMoveResult::MoveType::ppt)
            command_controller->try_resize_ppt(adjust.direction, adjust.first->amount, command_list.scope);
        else
            command_controller->try_resize(adjust.direction, adjust.first->amount, command_list.scope);
        return IpcValidationResult::create_success();
    }
    else if (arg0 == "set")
    {
        auto const result = parse_set_resize(indexer);
        if (!result.success)
            return IpcValidationResult::create_failure(result.error, true);

        command_controller->try_set_size(
            result.width.amount,
            result.width.move_type == ParseMoveResult::MoveType::ppt,
            result.height.amount,
            result.height.move_type == ParseMoveResult::MoveType::ppt,
            command_list.scope);
        return IpcValidationResult::create_success();
    }
    else
        return IpcValidationResult::create_failure(std::format("Encountered unexpected argument during 'resize': {}", arg0.c_str()), true);
}

IpcValidationResult IpcCommandExecutor::process_reload(IpcCommand const& command, IpcParseResult const&)
{
    if (!command.arguments.empty())
        return IpcValidationResult::create_failure("'reload' command expects no arguments", true);

    command_controller->reload_config();
    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_mark(IpcCommand const& command, IpcParseResult const& parse_result)
{
    ArgumentsIndexer indexer(command);
    if (!indexer.has_current())
        return IpcValidationResult::create_failure("'mark' command expects <identifier>", true);

    auto const& identifier = indexer.current();
    auto const is_adding = std::ranges::find(command.options, "--add") != command.options.end();
    auto const is_replacing = std::ranges::find(command.options, "--replace") != command.options.end();
    if (is_adding && is_replacing)
        return IpcValidationResult::create_failure("'mark' command expects either --add or --replace, not both", true);

    auto const is_toggling = std::ranges::find(command.options, "--toggle") != command.options.end();
    command_controller->mark(parse_result.scope, identifier, is_adding, is_toggling);
    return IpcValidationResult::create_success();
}

IpcValidationResult IpcCommandExecutor::process_unmark(IpcCommand const& command, IpcParseResult const& parse_result)
{
    ArgumentsIndexer indexer(command);
    if (indexer.has_current())
        command_controller->unmark(parse_result.scope, indexer.current());
    else
        command_controller->unmark_all(parse_result.scope);
    return IpcValidationResult::create_success();
}
