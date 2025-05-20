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

#ifndef MIRACLEWM_I_3_COMMAND_EXECUTOR_H
#define MIRACLEWM_I_3_COMMAND_EXECUTOR_H

#include "ipc_command.h"
#include <mir/glib_main_loop.h>

namespace miracle
{

class CommandController;
class AutoRestartingLauncher;
class WindowController;
class OutputManager;

struct IpcValidationResult
{
private:
    IpcValidationResult(bool success, bool parse_error, std::string error) :
        success(success),
        parse_error(parse_error),
        error(std::move(error))
    {
    }

public:
    IpcValidationResult() = delete;
    static IpcValidationResult create_success()
    {
        return IpcValidationResult(
            true,
            false,
            "");
    }

    static IpcValidationResult create_failure(std::string error, bool parse_error)
    {
        return IpcValidationResult(
            false,
            parse_error,
            std::move(error));
    }

    bool const success;
    bool const parse_error;
    std::string const error;
};

/// Processes all commands coming from i3 IPC. This class is mostly for organizational
/// purposes, as a lot of logic is associated with processing these operations.
class IpcCommandExecutor
{
public:
    IpcCommandExecutor(
        std::shared_ptr<CommandController> const&,
        AutoRestartingLauncher&,
        std::shared_ptr<WindowController> const&);
    std::vector<IpcValidationResult> process(IpcParseResult const&);

private:
    std::shared_ptr<CommandController> policy;
    AutoRestartingLauncher& launcher;
    std::shared_ptr<WindowController> window_controller;

    IpcValidationResult process_exec(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_split(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_focus(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_move(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_sticky(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_input(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_workspace(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_layout(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_scratchpad(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_resize(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_reload(IpcCommand const&, IpcParseResult const&);
};

} // miracle

#endif // MIRACLEWM_I_3_COMMAND_EXECUTOR_H
