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
#include "gaps.h"
#include "modifiers.h"

#include <array>
#include <cstdlib>
#include <glm/glm.hpp>
#include <mir_toolkit/events/enums.h>
#include <miral/input_configuration.h>
#include <miral/version.h>
#include <optional>
#include <string>
#include <vector>

namespace miracle
{
struct MIRACLE_WM_CONFIG_API KeyCommand
{
    MirKeyboardAction action;
    uint modifiers;
    uint key;
};

struct MIRACLE_WM_CONFIG_API CustomKeyCommand : KeyCommand
{
    std::string command;
};

struct MIRACLE_WM_CONFIG_API BuiltInKeyCommandOverride : KeyCommand
{
    DefaultKeyCommand default_key_command;
};

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
    float radius = 8.f;
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

struct MIRACLE_WM_CONFIG_API KeymapConfiguration
{
    std::string language;
    std::optional<std::string> variant;
    std::vector<std::string> options;

    [[nodiscard]] std::string to_string() const;
};

struct MIRACLE_WM_CONFIG_API ConfigData
{
    ConfigData();
    uint primary_modifier = mir_input_event_modifier_meta;
    uint primary_button = mir_pointer_button_primary;
    std::vector<CustomKeyCommand> custom_key_commands;
    std::vector<BuiltInKeyCommandOverride> built_in_key_command_overrides;
    Gaps inner_gaps = { .top = 10, .bottom = 10, .left = 10, .right = 10 };
    Gaps outer_gaps = { .top = 10, .bottom = 10, .left = 10, .right = 10 };
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
    miral::InputConfiguration::Mouse mouse_configuration;
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
    miral::InputConfiguration::Keyboard keyboard_configuration;
#endif
    std::optional<KeymapConfiguration> keymap;
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

struct MIRACLE_WM_CONFIG_API ConfigSaveResult
{
    bool success;
    std::vector<Error> errors;
};

/// Loads the configuration from the provided [path] and returns the loaded
/// configuration along with any errors that were found.
/// \returns Configuration alongside found errors
MIRACLE_WM_CONFIG_API ConfigLoadResult load_config(std::string const& path);

/// Save the configuration to the provided [path] and returns information about
/// the success of the save
/// \returns [ConfigSaveResult]
MIRACLE_WM_CONFIG_API ConfigSaveResult save_config(std::string const& path, ConfigData const& config);

MIRACLE_WM_CONFIG_API std::string get_config_path();

MIRACLE_WM_CONFIG_API std::string get_display_config_path();

}

#endif // MIRACLE_WM_CONFIG_H
