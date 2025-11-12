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

#ifndef MIRACLE_WM_CONFIG_TOUCHPAD_H
#define MIRACLE_WM_CONFIG_TOUCHPAD_H

#include <array>
#include <cstdlib>
#include <mir_toolkit/mir_input_device_types.h>

namespace miracle
{
constexpr std::array<std::pair<const char*, uint>, 3> mir_touchpad_click_mode_opts = {
    std::pair { "none",          mir_touchpad_click_mode_none          },
    std::pair { "area_to_click", mir_touchpad_click_mode_area_to_click },
    std::pair { "finger_count",  mir_touchpad_click_mode_finger_count  }
};

constexpr std::array<std::pair<const char*, uint>, 4> mir_touchpad_scroll_mode_opts = {
    std::pair { "none",               mir_touchpad_scroll_mode_none               },
    std::pair { "two_finger_scroll",  mir_touchpad_scroll_mode_two_finger_scroll  },
    std::pair { "edge_scroll",        mir_touchpad_scroll_mode_edge_scroll        },
    std::pair { "button_down_scroll", mir_touchpad_scroll_mode_button_down_scroll }
};

}

#endif // MIRACLE_WM_CONFIG_TOUCHPAD_H
