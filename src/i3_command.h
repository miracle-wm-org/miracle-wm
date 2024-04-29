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

#ifndef MIRACLEWM_I3_COMMAND_H
#define MIRACLEWM_I3_COMMAND_H

#include <vector>
#include <string>
#include <optional>

namespace miracle
{
enum class I3CommandType
{
    NONE,
    EXEC,
    SPLIT,
    LAYOUT,
    FOCUS,
    MOVE,
    SWAP,
    STICKY,
    WORKSPACE,
    MARK,
    TITLE_FORMAT,
    TITLE_WINDOW_ICON,
    BORDER,
    SHM_LOG,
    DEBUG_LOG,
    RESTART,
    RELOAD,
    EXIT,
    SCRATCHPAD,
    NOP,
    I3_BAR,
    GAPS
};

struct I3Scope
{

};

struct I3Command
{
    I3CommandType type = I3CommandType::NONE;
    std::optional<I3Scope> scope;
    std::vector<std::string> arguments;
};
}

#endif //MIRACLEWM_I3_COMMAND_H
