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

#ifndef MIRACLE_WM_CONFIG_H
#define MIRACLE_WM_CONFIG_H

#include "animation_definition.h"
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
#include <mir_toolkit/mir_input_device_types.h>
#include <miral/input_configuration.h>
#include <miral/version.h>
#include <optional>
#include <string>
#include <vector>

namespace miracle
{
struct MIRACLE_WM_CONFIG_API CustomKeyCommand
{
    MirKeyboardAction action;
    uint modifiers;
    uint key;
    std::string command;
};

struct MIRACLE_WM_CONFIG_API BuiltInKeyCommandOverride
{
    MirKeyboardAction action;
    uint modifiers;
    uint key;
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

struct MIRACLE_WM_CONFIG_API MagnifierConfiguration
{
    bool enabled = false;
    float scale = 1.5f;
    float scale_increment = 0.5f;
    int width = 400;
    int height = 400;
    int size_increment = 50;
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
    CursorFocusMode focus_mode = CursorFocusMode::Click;
    std::optional<std::string> theme = std::nullopt;

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

struct MIRACLE_WM_CONFIG_API TouchpadConfiguration
{
    // Follows the pattern from miral::InputConfiguration::Touchpad
    bool disable_while_typing = false;
    bool disable_with_external_mouse = false;
    float acceleration_bias = 0.0f;
    float vscroll_speed = 1.0f;
    float hscroll_speed = 1.0f;
    bool tap_to_click = true;
    bool middle_mouse_button_emulation = false;
    MirTouchpadClickMode click_mode = mir_touchpad_click_mode_none;
    MirTouchpadScrollMode scroll_mode = mir_touchpad_scroll_mode_two_finger_scroll;

    bool operator==(const TouchpadConfiguration&) const = default;
};

struct MIRACLE_WM_CONFIG_API PluginConfiguration
{
    std::string path;
    std::string userdata_json;
};

// Forward declaration so ConfigData::merge_with_plugin_config can reference PluginConfigData.
struct PluginConfigData;

/// Configuration for the helper clients that ship with miracle-wm. These are
/// internal clients that the compositor launches on the user's behalf (e.g. the
/// error reporter). This struct is also where future internal clients (decorations,
/// top bars, etc.) will be configured.
struct MIRACLE_WM_CONFIG_API WmClientsConfig
{
    /// Selects the client used to display configuration errors. Accepts:
    ///   "default"  - use the bundled miracle-wm-basic-error-reporter
    ///   "disabled" - do not launch any error reporter
    ///   <string>   - path or name of an executable to launch instead
    std::string error_reporter = "default";
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
    miracle::WithDefaultFlag<TouchpadConfiguration> touchpad;
    miracle::WithDefaultFlag<MagnifierConfiguration> magnifier;
    miracle::WithDefaultFlag<bool> workspace_back_and_forth = true;
    miracle::WithDefaultFlag<std::vector<PluginConfiguration>> plugins;
    miracle::WithDefaultFlag<WmClientsConfig> wm_clients;

    /// Other configuration files to include in addition to this one.
    miracle::WithDefaultFlag<std::vector<std::string>> includes;

    ConfigData merge_with(ConfigData& other);
    ConfigData merge_with_plugin_config(PluginConfigData const& other);
    static AnimationDefinition get_default_animation_definition(AnimateableEvent event);
};

/// A subset of ConfigData that plugins may return from their configure() hook.
///
/// Every field is optional (uses WithDefaultFlag). Plugins cannot set the
/// 'plugins' or 'includes' fields — those are excluded entirely from this struct.
struct MIRACLE_WM_CONFIG_API PluginConfigData
{
    miracle::WithDefaultFlag<uint> primary_modifier;
    miracle::WithDefaultFlag<uint> primary_button;
    miracle::WithDefaultFlag<std::vector<CustomKeyCommand>> custom_key_commands;
    miracle::WithDefaultFlag<std::vector<BuiltInKeyCommandOverride>> built_in_key_command_overrides;
    miracle::WithDefaultFlag<Gaps> inner_gaps;
    miracle::WithDefaultFlag<Gaps> outer_gaps;
    miracle::WithDefaultFlag<std::vector<StartupApp>> startup_apps;
    miracle::WithDefaultFlag<std::optional<std::string>> terminal;
    miracle::WithDefaultFlag<int> resize_jump;
    miracle::WithDefaultFlag<std::vector<EnvironmentVariable>> environment_variables;
    miracle::WithDefaultFlag<BorderConfig> border_config;
    miracle::WithDefaultFlag<bool> animations_enabled;
    miracle::WithDefaultFlag<std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)>> animation_definitions;
    miracle::WithDefaultFlag<std::vector<WorkspaceConfig>> workspace_configs;
    miracle::WithDefaultFlag<uint> move_modifier;
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
    miracle::WithDefaultFlag<TouchpadConfiguration> touchpad;
    miracle::WithDefaultFlag<MagnifierConfiguration> magnifier;
    miracle::WithDefaultFlag<bool> workspace_back_and_forth;
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

struct MIRACLE_WM_CONFIG_API PluginConfigLoadResult
{
    PluginConfigData config;
    std::vector<Error> errors;
};

/// Loads the configuration from the provided [path] and returns the loaded
/// configuration along with any errors that were found.
/// \returns Configuration alongside found errors
MIRACLE_WM_CONFIG_API ConfigLoadResult load_config(std::string const& path);

/// Parse a JSON (or YAML) string into a PluginConfigData.
///
/// Uses the same per-field readers as load_config(). The 'plugins' and
/// 'includes' keys are silently ignored even if present in the input.
MIRACLE_WM_CONFIG_API PluginConfigLoadResult load_plugin_config_from_string(std::string const& json);

/// Save the configuration to the provided [path] and returns information about
/// the success of the save
/// \returns [ConfigSaveResult]
MIRACLE_WM_CONFIG_API ConfigSaveResult save_config(std::string const& path, ConfigData const& config);

MIRACLE_WM_CONFIG_API std::string get_config_path();

MIRACLE_WM_CONFIG_API std::optional<std::string> read_cursor_theme_from_file(std::string const& path);

MIRACLE_WM_CONFIG_API std::string get_user_config_dir();

MIRACLE_WM_CONFIG_API std::string get_display_config_path();

}

#endif // MIRACLE_WM_CONFIG_H
