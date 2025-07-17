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

#ifndef MIRACLEWM_BINDING_EVENT_H
#define MIRACLEWM_BINDING_EVENT_H

#include <nlohmann/json.hpp>
#include <string>
#include <xkbcommon/xkbcommon.h>

namespace miracle
{

enum class BindingEventType
{
    keyboard,
    mouse
};

class BindingEvent
{
public:
    BindingEvent(std::string const& mode, std::string const& command, unsigned int modifiers, xkb_keysym_t keysym, BindingEventType type);
    [[nodiscard]] nlohmann::json to_json() const;

private:
    std::string const change = "run";
    std::string const mode;
    std::string command;
    unsigned int const modifiers;
    xkb_keysym_t const keysym;
    BindingEventType type;
};

}

#endif
