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

#ifndef MIRACLE_WM_CONFIG_DEFAULT_KEY_COMMAND_H
#define MIRACLE_WM_CONFIG_DEFAULT_KEY_COMMAND_H

#include "export.h"
#include <array>

namespace miracle
{
enum class MIRACLE_WM_CONFIG_API DefaultKeyCommand
{
    Terminal = 0,
    RequestVertical,
    RequestHorizontal,
    ToggleResize,
    ResizeUp,
    ResizeDown,
    ResizeLeft,
    ResizeRight,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    SelectUp,
    SelectDown,
    SelectLeft,
    SelectRight,
    QuitActiveWindow,
    QuitCompositor,
    Fullscreen,
    SelectWorkspace1,
    SelectWorkspace2,
    SelectWorkspace3,
    SelectWorkspace4,
    SelectWorkspace5,
    SelectWorkspace6,
    SelectWorkspace7,
    SelectWorkspace8,
    SelectWorkspace9,
    SelectWorkspace0,
    MoveToWorkspace1,
    MoveToWorkspace2,
    MoveToWorkspace3,
    MoveToWorkspace4,
    MoveToWorkspace5,
    MoveToWorkspace6,
    MoveToWorkspace7,
    MoveToWorkspace8,
    MoveToWorkspace9,
    MoveToWorkspace0,
    ToggleFloating,
    TogglePinnedToWorkspace,
    ToggleTabbing,
    ToggleStacking,
    MagnifierOn,
    MagnifierOff,
    MagnifierIncreaseSize,
    MagnifierDecreaseSize,
    MagnifierIncreaseScale,
    MagnifierDecreaseScale,
    MAX
};

constexpr std::array<const char*, static_cast<int>(DefaultKeyCommand::MAX)> default_key_command_strings = {
    "terminal",
    "request_vertical_layout",
    "request_horizontal_layout",
    "toggle_resize",
    "resize_up",
    "resize_down",
    "resize_left",
    "resize_right",
    "move_up",
    "move_down",
    "move_left",
    "move_right",
    "select_up",
    "select_down",
    "select_left",
    "select_right",
    "quit_active_window",
    "quit_compositor",
    "fullscreen",
    "select_workspace_1",
    "select_workspace_2",
    "select_workspace_3",
    "select_workspace_4",
    "select_workspace_5",
    "select_workspace_6",
    "select_workspace_7",
    "select_workspace_8",
    "select_workspace_9",
    "select_workspace_0",
    "move_to_workspace_1",
    "move_to_workspace_2",
    "move_to_workspace_3",
    "move_to_workspace_4",
    "move_to_workspace_5",
    "move_to_workspace_6",
    "move_to_workspace_7",
    "move_to_workspace_8",
    "move_to_workspace_9",
    "move_to_workspace_0",
    "toggle_floating",
    "toggle_pinned_to_workspace",
    "toggle_tabbing",
    "toggle_stacking",
    "magnifierOn",
    "magnifierOff",
    "magnifierIncreaseSize",
    "magnifierDecreaseSize",
    "magnifierIncreaseScale",
    "magnifierDecreaseScale"
};

}

#endif // MIRACLE_WM_CONFIG_DEFAULT_KEY_COMMAND_H
