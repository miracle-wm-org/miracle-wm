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

#ifndef MIRACLEWM_I_3_COMMAND_EXECUTOR_H
#define MIRACLEWM_I_3_COMMAND_EXECUTOR_H

#include "ipc_command.h"
#include <mir/glib_main_loop.h>
#include <mir/synchronised.h>

namespace miracle
{

class AbstractCommandController;
class Launcher;
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
        return {
            true,
            false,
            ""
        };
    }

    static IpcValidationResult create_failure(std::string error, bool parse_error)
    {
        return {
            false,
            parse_error,
            std::move(error)
        };
    }

    bool const success;
    bool const parse_error;
    std::string const error;
};

class AbstractIpcCommandExecutor
{
public:
    virtual ~AbstractIpcCommandExecutor() = default;
    virtual std::vector<IpcValidationResult> process(IpcParseResult const&) = 0;

    /// Applies any startup commands to the [container] if any exist.
    virtual void apply_startup_commands_to(std::shared_ptr<Container> const& container) = 0;
};

/// Processes all commands coming from i3 IPC. This class is mostly for organizational
/// purposes, as a lot of logic is associated with processing these operations.
class IpcCommandExecutor : public AbstractIpcCommandExecutor
{
public:
    IpcCommandExecutor(
        std::shared_ptr<AbstractCommandController> const&,
        std::shared_ptr<Launcher> const&);
    std::vector<IpcValidationResult> process(IpcParseResult const&) override;
    void apply_startup_commands_to(std::shared_ptr<Container> const& container) override;

private:
    std::shared_ptr<AbstractCommandController> command_controller;
    std::shared_ptr<Launcher> launcher;
    mir::Synchronised<std::vector<IpcParseResult>> apply_on_startup;

    IpcValidationResult process_exec(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_split(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_focus(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_move(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_swap(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_sticky(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_input(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_workspace(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_layout(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_fullscreen(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_floating(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_scratchpad(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_resize(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_reload(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_mark(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_unmark(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_rename(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_gap(IpcCommand const&, IpcParseResult const&) const;
    IpcValidationResult process_nop(IpcCommand const&, IpcParseResult const&) const;
};

} // miracle

#endif // MIRACLEWM_I_3_COMMAND_EXECUTOR_H
