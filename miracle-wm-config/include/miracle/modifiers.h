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

#ifndef MIRACLE_WM_CONFIG_MODIFIERS_H
#define MIRACLE_WM_CONFIG_MODIFIERS_H

#include <array>
#include <mir_toolkit/events/enums.h>

namespace miracle
{

constexpr std::array<const char*, 18> mir_input_event_modifier_strings = {
    "none",
    "alt",
    "alt_left",
    "alt_right",
    "shift",
    "shift_left",
    "shift_right",
    "sym",
    "function",
    "ctrl",
    "ctrl_left",
    "ctrl_right",
    "meta",
    "meta_left",
    "meta_right",
    "caps_lock",
    "num_lock",
    "scroll_lock"
};

}

#endif // MIRACLE_WM_CONFIG_MODIFIERS_H
