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

#ifndef MIRACLE_WM_CONFIG_H
#define MIRACLE_WM_CONFIG_H

#include "animation_definition.h"
#include "container_type.h"
#include "default_key_command.h"
#include "export.h"
#include "keyboard.h"
#include "modifiers.h"

#include <array>
#include <cstdlib>
#include <glm/glm.hpp>
#include <mir_toolkit/events/enums.h>
#include <optional>
#include <string>
#include <vector>

namespace miracle
{
constexpr uint miracle_input_event_modifier_default = 1 << 18;

struct MIRACLE_WM_CONFIG_API KeyCommand
{
    MirKeyboardAction action;
    uint modifiers;
    int key;
};

struct MIRACLE_WM_CONFIG_API CustomKeyCommand : KeyCommand
{
    std::string command;
};

typedef std::vector<KeyCommand> KeyCommandList;

struct MIRACLE_WM_CONFIG_API StartupApp
{
    std::string command;
    bool restart_on_death = false;
    bool no_startup_id = false;
    bool should_halt_compositor_on_death = false;
    bool in_systemd_scope = false;
};

struct MIRACLE_WM_CONFIG_API EnvironmentVariable
{
    std::string key;
    std::string value;
};

struct MIRACLE_WM_CONFIG_API BorderConfig
{
    int size = 0;
    glm::vec4 focus_color = glm::vec4(0);
    glm::vec4 color = glm::vec4(0);
};

struct MIRACLE_WM_CONFIG_API WorkspaceConfig
{
    std::optional<int> num;
    std::optional<ContainerType> layout;
    std::optional<std::string> name;
};

enum class MIRACLE_WM_CONFIG_API RenderFilter : int
{
    none,
    grayscale,
    protanopia,
    deuteranopia,
    tritanopia
};

struct MIRACLE_WM_CONFIG_API DragAndDropConfiguration
{
    bool enabled = true;
    uint modifiers = miracle_input_event_modifier_default | mir_input_event_modifier_shift;
};

struct MIRACLE_WM_CONFIG_API ConfigData
{
    ConfigData();
    uint primary_modifier = mir_input_event_modifier_meta;
    uint primary_button = mir_pointer_button_primary;
    std::vector<CustomKeyCommand> custom_key_commands;
    KeyCommandList key_commands[static_cast<int>(DefaultKeyCommand::MAX)];
    int inner_gaps_x = 10;
    int inner_gaps_y = 10;
    int outer_gaps_x = 10;
    int outer_gaps_y = 10;
    std::vector<StartupApp> startup_apps;
    std::optional<std::string> terminal = "miracle-wm-sensible-terminal";
    int resize_jump = 50;
    std::vector<EnvironmentVariable> environment_variables;
    BorderConfig border_config;
    bool animations_enabled = true;
    std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> animation_definitions;
    std::vector<WorkspaceConfig> workspace_configs;
    uint move_modifier = miracle_input_event_modifier_default;
    DragAndDropConfiguration drag_and_drop;
};

enum class ErrorLevel
{
    warning,
    error
};

struct MIRACLE_WM_CONFIG_API Error
{
    int const line;
    int const column;
    ErrorLevel const level;
    std::string const filename;
    std::string const message;
};

struct MIRACLE_WM_CONFIG_API ConfigLoadResult
{
    ConfigData config;
    std::vector<Error> errors;
};

/// Loads the configuration from the provided [path] and returns the loaded
/// configuration along with any errors that were found.
/// \returns Configuration alongside found errors
MIRACLE_WM_CONFIG_API ConfigLoadResult load_config(std::string const& path);

}

#endif // MIRACLE_WM_CONFIG_H
