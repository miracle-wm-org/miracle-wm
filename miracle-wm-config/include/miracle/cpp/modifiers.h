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

#ifndef MIRACLE_WM_CONFIG_MODIFIERS_H
#define MIRACLE_WM_CONFIG_MODIFIERS_H

#include <array>
#include <cstdlib>
#include <mir_toolkit/events/enums.h>

namespace miracle
{
constexpr uint miracle_input_event_modifier_default = 1 << 18;

constexpr std::array<std::pair<const char*, uint>, 18> mir_input_event_modifier_opts = {
    std::pair { "alt",         mir_input_event_modifier_alt         },
    std::pair { "alt_left",    mir_input_event_modifier_alt_left    },
    std::pair { "alt_right",   mir_input_event_modifier_alt_right   },
    std::pair { "shift",       mir_input_event_modifier_shift       },
    std::pair { "shift_left",  mir_input_event_modifier_shift_left  },
    std::pair { "shift_right", mir_input_event_modifier_shift_right },
    std::pair { "sym",         mir_input_event_modifier_sym         },
    std::pair { "function",    mir_input_event_modifier_function    },
    std::pair { "ctrl",        mir_input_event_modifier_ctrl        },
    std::pair { "ctrl_left",   mir_input_event_modifier_ctrl_left   },
    std::pair { "ctrl_right",  mir_input_event_modifier_ctrl_right  },
    std::pair { "meta",        mir_input_event_modifier_meta        },
    std::pair { "meta_left",   mir_input_event_modifier_meta_left   },
    std::pair { "meta_right",  mir_input_event_modifier_meta_right  },
    std::pair { "caps_lock",   mir_input_event_modifier_caps_lock   },
    std::pair { "num_lock",    mir_input_event_modifier_num_lock    },
    std::pair { "scroll_lock", mir_input_event_modifier_scroll_lock },
    std::pair { "primary",     miracle_input_event_modifier_default }
};

}

#endif // MIRACLE_WM_CONFIG_MODIFIERS_H
