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

#ifndef MIRACLE_WM_CONFIG_MOUSE_BUTTON_H
#define MIRACLE_WM_CONFIG_MOUSE_BUTTON_H

#include <array>
#include <cstdlib>
#include <mir_toolkit/events/enums.h>

namespace miracle
{
constexpr std::array<std::pair<const char*, uint>, 8> mir_mouse_buttons_opts = {
    std::pair { "primary",   mir_pointer_button_primary   },
    std::pair { "secondary", mir_pointer_button_secondary },
    std::pair { "tertiary",  mir_pointer_button_tertiary  },
    std::pair { "back",      mir_pointer_button_back      },
    std::pair { "forward",   mir_pointer_button_forward   },
    std::pair { "side",      mir_pointer_button_side      },
    std::pair { "extra",     mir_pointer_button_extra     },
    std::pair { "task",      mir_pointer_button_task      }
};

}

#endif // MIRACLE_WM_CONFIG_MOUSE_BUTTON_H
