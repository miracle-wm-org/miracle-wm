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

#ifndef MIRACLE_WM_CONFIG_KEYBOARD_H
#define MIRACLE_WM_CONFIG_KEYBOARD_H

#include <array>
#include <cstdlib>
#include <mir_toolkit/events/enums.h>

namespace miracle
{
constexpr std::array<std::pair<const char*, uint>, mir_keyboard_actions - 1> mir_keyboard_actions_strings = {
    std::pair { "up",     mir_keyboard_action_up     },
    std::pair { "down",   mir_keyboard_action_down   },
    std::pair { "repeat", mir_keyboard_action_repeat }
};
}

#endif // MIRACLE_WM_CONFIG_KEYBOARD_H
