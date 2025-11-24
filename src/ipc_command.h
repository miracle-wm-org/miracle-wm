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

#ifndef MIRACLEWM_I3_COMMAND_H
#define MIRACLEWM_I3_COMMAND_H

#include "container_scope.h"

#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <string>
#include <vector>

namespace miracle
{
class WindowController;

enum class IpcCommandType
{
    none,
    exec,
    split,
    layout,
    fullscreen,
    floating,
    focus,
    move,
    swap,
    sticky,
    workspace,
    mark,
    unmark,
    title_format,
    title_window_icon,
    border,
    shm_log,
    debug_log,
    restart,
    reload,
    exit,
    scratchpad,
    nop,
    i3_bar,
    gaps,
    input,
    resize,
    rename
};

struct IpcCommand
{
    IpcCommandType const type;
    std::string const raw_command;
    std::vector<std::string> options;
    std::vector<std::string> arguments;
};

struct IpcParseResult
{
    bool for_window = false;
    std::vector<ContainerScope> scope;
    std::vector<IpcCommand> commands;
};

class IpcCommandParser
{
public:
    explicit IpcCommandParser(const char*);
    IpcParseResult parse();

private:
    enum class ParseState
    {
        root,
        scope_key,
        scope_value,
        literal,
        command,
        option,
        argument,
        for_window
    };

    std::string data;
    std::vector<ParseState> stack = { ParseState::root };
    size_t index = 0;
    bool has_parsed_command = false;
    bool can_parse_options = true;
    size_t scope_num_before = 0;
};
}

#endif // MIRACLEWM_I3_COMMAND_H
