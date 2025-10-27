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
#include "cursor_focus_mode.h"
#include "default_key_command.h"
#include "export.h"
#include "gaps.h"
#include "modifiers.h"
#include "with_default_flag.h"

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

struct MIRACLE_WM_CONFIG_API HoverClickConfiguration
{
    bool enabled = false;
    uint hover_duration_milliseconds = 1000;
    int cancel_displacement_threshold = 10;
    int reclick_displacement_threshold = 5;

    bool operator==(const HoverClickConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API SimulatedSecondaryClickConfiguration
{
    bool enabled = false;
    uint hold_duration_milliseconds = 1000;
    int displacement_threshold = 20;

    bool operator==(const SimulatedSecondaryClickConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API OutputFilterConfiguration
{
    std::optional<std::string> shader_path;

    bool operator==(const OutputFilterConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API CursorConfiguration
{
    float scale = 1.f;

    bool operator==(const CursorConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API SlowKeysConfiguration
{
    bool enabled = false;
    uint hold_delay_milliseconds = 200;

    bool operator==(const SlowKeysConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API StickyKeysConfiguration
{
    bool enabled = false;
    bool should_disable_if_two_keys_are_pressed_together = true;

    bool operator==(const StickyKeysConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API CursorFocusConfiguration
{
  CursorFocusMode mode = CursorFocusMode::HoverFocus;
};

struct MIRACLE_WM_CONFIG_API ConfigData
{
    ConfigData();
    miracle::WithDefaultFlag<uint> primary_modifier = mir_input_event_modifier_meta;
    miracle::WithDefaultFlag<uint> primary_button = mir_pointer_button_primary;
    miracle::WithDefaultFlag<std::vector<CustomKeyCommand>> custom_key_commands;
    miracle::WithDefaultFlag<std::vector<BuiltInKeyCommandOverride>> built_in_key_command_overrides;
    miracle::WithDefaultFlag<Gaps> inner_gaps = Gaps { .top = 10, .bottom = 10, .left = 10, .right = 10 };
    miracle::WithDefaultFlag<Gaps> outer_gaps = Gaps { .top = 10, .bottom = 10, .left = 10, .right = 10 };
    miracle::WithDefaultFlag<std::vector<StartupApp>> startup_apps;
    miracle::WithDefaultFlag<std::optional<std::string>> terminal = std::optional<std::string>("miracle-wm-sensible-terminal");
    miracle::WithDefaultFlag<int> resize_jump = 50;
    miracle::WithDefaultFlag<std::vector<EnvironmentVariable>> environment_variables;
    miracle::WithDefaultFlag<BorderConfig> border_config;
    miracle::WithDefaultFlag<bool> animations_enabled = true;
    miracle::WithDefaultFlag<std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)>> animation_definitions;
    miracle::WithDefaultFlag<std::vector<WorkspaceConfig>> workspace_configs;
    miracle::WithDefaultFlag<uint> move_modifier = miracle_input_event_modifier_default;
    miracle::WithDefaultFlag<DragAndDropConfiguration> drag_and_drop;
    miracle::WithDefaultFlag<miral::InputConfiguration::Mouse> mouse_configuration;
    miracle::WithDefaultFlag<miral::InputConfiguration::Keyboard> keyboard_configuration;
    miracle::WithDefaultFlag<std::optional<KeymapConfiguration>> keymap;
    miracle::WithDefaultFlag<HoverClickConfiguration> hover_click;
    miracle::WithDefaultFlag<SimulatedSecondaryClickConfiguration> simulated_secondary_click;
    miracle::WithDefaultFlag<OutputFilterConfiguration> output_filter;
    miracle::WithDefaultFlag<CursorConfiguration> cursor;
    miracle::WithDefaultFlag<SlowKeysConfiguration> slow_keys;
    miracle::WithDefaultFlag<StickyKeysConfiguration> sticky_keys;
    miracle::WithDefaultFlag<CursorFocusConfiguration> cursor_focus;

    /// Other configuration files to include in addition to this one.
    miracle::WithDefaultFlag<std::vector<std::string>> includes;

    ConfigData merge_with(ConfigData& other);
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

MIRACLE_WM_CONFIG_API std::string get_user_config_dir();

MIRACLE_WM_CONFIG_API std::string get_display_config_path();

}

#endif // MIRACLE_WM_CONFIG_H
