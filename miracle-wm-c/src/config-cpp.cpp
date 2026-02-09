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

#include "miracle/cpp/config-cpp.h"
#include "miracle/cpp/cursor_focus_mode.h"
#include "miracle/cpp/gaps.h"
#include "miracle/cpp/keyboard.h"
#include "miracle/cpp/touchpad.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <glib-2.0/glib.h>
#include <glm/fwd.hpp>
#include <iostream>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <miral/version.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

namespace YAML
{
class BadConversion;
}

namespace
{
const std::array<miracle::AnimationDefinition, static_cast<int>(miracle::AnimateableEvent::max)> default_animation_definitions({
    { miracle::AnimationType::built_in,
     true,
     0.2f,
     miracle::BuiltInAnimationList { miracle::BuiltInAnimationDefinition {
            miracle::BultInAnimationType::fade,
            miracle::EaseFunction::linear,
        } } },
    { miracle::AnimationType::built_in,
     true,
     0.25f,
     miracle::BuiltInAnimationList { miracle::BuiltInAnimationDefinition {
            miracle::BultInAnimationType::slide,
            miracle::EaseFunction::linear,
        } } },
    { miracle::AnimationType::built_in,
     true,
     0.3f,
     miracle::BuiltInAnimationList { miracle::BuiltInAnimationDefinition {
            miracle::BultInAnimationType::fade,
            miracle::EaseFunction::linear,
        } } },
    { miracle::AnimationType::built_in,
     true,
     0.25f,
     miracle::BuiltInAnimationList { miracle::BuiltInAnimationDefinition {
            miracle::BultInAnimationType::slide,
            miracle::EaseFunction::ease_out_sine,
        } } }
});

struct ParsingContext
{
    miracle::ConfigLoadResult result;
    std::string path;
    std::stringstream builder;
};

std::optional<MirKeyboardAction> from_string_keyboard_action(std::string const& str, ParsingContext& context)
{
    for (auto const& [fst, snd] : miracle::mir_keyboard_actions_strings)
    {
        if (fst == str)
            return static_cast<MirKeyboardAction>(snd);
    }

    return std::nullopt;
}

std::optional<miracle::AnimateableEvent> from_string_animateable_event(std::string const& str, ParsingContext& context)
{
    for (size_t i = 0; i < miracle::animateable_event_strings.size(); i++)
    {
        if (miracle::animateable_event_strings[i] == str)
            return static_cast<miracle::AnimateableEvent>(i);
    }

    return std::nullopt;
}

std::optional<miracle::EaseFunction> from_string_ease_function(std::string const& str, ParsingContext& context)
{
    for (size_t i = 0; i < miracle::ease_function_strings.size(); i++)
    {
        if (miracle::ease_function_strings[i] == str)
            return static_cast<miracle::EaseFunction>(i);
    }

    return std::nullopt;
}

std::optional<miracle::AnimationType> from_string_animation_type(std::string const& str, ParsingContext& context)
{
    for (size_t i = 0; i < miracle::animation_type_strings.size(); i++)
    {
        if (miracle::animation_type_strings[i] == str)
            return static_cast<miracle::AnimationType>(i);
    }

    return std::nullopt;
}

std::optional<miracle::BultInAnimationType> from_string_built_in_animation_type(std::string const& str, ParsingContext& context)
{
    for (size_t i = 0; i < miracle::built_in_animation_type_strings.size(); i++)
    {
        if (miracle::built_in_animation_type_strings[i] == str)
            return static_cast<miracle::BultInAnimationType>(i);
    }

    return std::nullopt;
}

std::optional<MirPointerHandedness> from_string_handedness(std::string const& str, ParsingContext&)
{
    if (str == "left")
        return mir_pointer_handedness_left;
    else
        return mir_pointer_handedness_right;
}

std::string to_string_handedness(MirPointerHandedness handedness)
{
    switch (handedness)
    {
    case mir_pointer_handedness_left:
        return "left";
    default:
    case mir_pointer_handedness_right:
        return "right";
    }
}

std::optional<MirPointerAcceleration> from_string_acceleration(std::string const& str, ParsingContext&)
{
    if (str == "adaptive")
        return mir_pointer_acceleration_adaptive;
    else
        return mir_pointer_acceleration_none;
}

std::optional<miracle::CursorFocusMode> from_string_cursor_focus_mode(std::string const& str, ParsingContext&)
{
    for (size_t i = 0; i < miracle::cursor_focus_mode_strings.size(); i++)
    {
        if (miracle::cursor_focus_mode_strings[i] == str)
            return static_cast<miracle::CursorFocusMode>(i);
    }

    return std::nullopt;
}

std::string to_string_acceleration(MirPointerAcceleration acceleration)
{
    switch (acceleration)
    {
    case mir_pointer_acceleration_adaptive:
        return "adaptive";
    default:
    case mir_pointer_acceleration_none:
        return "none";
    }
}

std::optional<MirTouchpadClickMode> from_string_touchpad_click_mode(std::string const& str, ParsingContext&)
{
    for (const auto& [name, value] : miracle::mir_touchpad_click_mode_opts)
    {
        if (str == name)
            return static_cast<MirTouchpadClickMode>(value);
    }
    return std::nullopt;
}

std::string to_string_touchpad_click_mode(MirTouchpadClickMode click_mode)
{
    for (const auto& [name, value] : miracle::mir_touchpad_click_mode_opts)
    {
        if (static_cast<uint>(click_mode) == value)
            return name;
    }
    return "finger_count"; // default fallback
}

std::optional<MirTouchpadScrollMode> from_string_touchpad_scroll_mode(std::string const& str, ParsingContext&)
{
    for (const auto& [name, value] : miracle::mir_touchpad_scroll_mode_opts)
    {
        if (str == name)
            return static_cast<MirTouchpadScrollMode>(value);
    }
    return std::nullopt;
}

std::string to_string_touchpad_scroll_mode(MirTouchpadScrollMode scroll_mode)
{
    for (const auto& [name, value] : miracle::mir_touchpad_scroll_mode_opts)
    {
        if (static_cast<uint>(value) == static_cast<uint>(scroll_mode))
            return name;
    }
    return "two_finger_scroll"; // default fallback
}

int program_exists(std::string const& name)
{
    std::stringstream out;
    out << "command -v " << name << " > /dev/null 2>&1";
    return !system(out.str().c_str());
}

void create_error(YAML::Node const& node, ParsingContext& context)
{
    context.result.errors.push_back({ node.Mark().line,
        node.Mark().column,
        miracle::ErrorLevel::error,
        context.path,
        context.builder.str() });
    context.builder.str("");
}

std::optional<uint> try_parse_modifier(std::string const& str, ParsingContext& context)
{
    for (const auto& [fst, snd] : miracle::mir_input_event_modifier_opts)
    {
        if (fst == str)
            return snd;
    }

    return std::nullopt;
}

template <typename T>
T try_parse_string_to_optional_value_by_key(
    YAML::Node const& root,
    const char* key,
    std::function<T(std::string const&, ParsingContext& context)> const& parse,
    ParsingContext& context)
{
    if (!root[key])
    {
        context.builder << "Missing key in value: " << key;
        create_error(root, context);
        return std::nullopt;
    }

    return try_parse_string_to_optional_value(root[key], parse);
}

template <typename T>
T try_parse_string_to_optional_value(
    YAML::Node const& node,
    std::function<T(std::string const&, ParsingContext& context)> const& parse,
    ParsingContext& context)
{
    try
    {
        auto const& v = node.as<std::string>();
        T retval = parse(v, context);
        if (retval == std::nullopt)
        {
            context.builder << "Invalid option: " << v;
            create_error(node, context);
        }

        return retval;
    }
    catch (YAML::BadConversion const& e)
    {
        context.builder << "Unable to parse enum value";
        create_error(node, context);
        return std::nullopt;
    }
}

template <typename T>
T try_parse_string_to_optional_value(YAML::Node const& root, const char* key, std::function<T(std::string const&, ParsingContext& context)> const& parse, ParsingContext& context)
{
    if (!root[key])
    {
        context.builder << "Missing key in value: " << key;
        create_error(root, context);
        return std::nullopt;
    }

    return try_parse_string_to_optional_value(root[key], parse, context);
}

template <typename T>
bool try_parse_value(YAML::Node const& node, T& value, ParsingContext& context)
{
    try
    {
        value = node.as<T>();
    }
    catch (YAML::BadConversion const& e)
    {
        context.builder << "Unable to parse value to correct type";
        create_error(node, context);
        return false;
    }
    return true;
}

template <typename T>
bool try_parse_value(YAML::Node const& root, const char* key, T& value, ParsingContext& context, bool optional = false)
{
    if (!root[key])
    {
        if (!optional)
        {
            context.builder << "Node is missing key: " << key;
            create_error(root, context);
        }
        return false;
    }

    return try_parse_value(root[key], value, context);
}

bool try_parse_modifiers(YAML::Node const& node, uint& modifiers, ParsingContext& context)
{
    if (!node.IsSequence())
    {
        context.builder << "Modifiers list must be an array";
        create_error(node, context);
        return false;
    }

    modifiers = 0;
    for (auto const& modifier_item : node)
    {
        if (auto const modifier = try_parse_string_to_optional_value<std::optional<uint>>(modifier_item, try_parse_modifier, context))
            modifiers = modifiers | modifier.value();
        else
        {
            context.builder << "Modifier is invalid";
            create_error(modifier_item, context);
            return false;
        }
    }

    return true;
}

bool try_parse_color(YAML::Node const& node, glm::vec4& color, ParsingContext& context)
{
    constexpr float MAX_COLOR_VALUE = 255;
    float r, g, b, a;
    if (node.IsMap())
    {
        if (!try_parse_value(node, "r", r, context))
            return false;

        if (!try_parse_value(node, "g", g, context))
            return false;

        if (!try_parse_value(node, "b", b, context))
            return false;

        if (!try_parse_value(node, "a", a, context))
            return false;

        r = r / MAX_COLOR_VALUE;
        g = g / MAX_COLOR_VALUE;
        b = b / MAX_COLOR_VALUE;
        a = a / MAX_COLOR_VALUE;
    }
    else if (node.IsSequence())
    {
        if (node.size() != 4)
        {
            context.builder << "Expected color values to be an array of size 4";
            create_error(node, context);
            return false;
        }

        // Parse as [r, g, b, a] array
        r = node[0].as<float>() / MAX_COLOR_VALUE;
        g = node[1].as<float>() / MAX_COLOR_VALUE;
        b = node[2].as<float>() / MAX_COLOR_VALUE;
        a = node[3].as<float>() / MAX_COLOR_VALUE;
    }
    else
    {
        // Parse as hex color
        std::string value;
        if (!try_parse_value(node, value, context))
            return false;

        try
        {
            unsigned long const i = std::stoul(value, nullptr, 16);
            r = static_cast<float>(((i >> 24) & 0xFF)) / MAX_COLOR_VALUE;
            g = static_cast<float>(((i >> 16) & 0xFF)) / MAX_COLOR_VALUE;
            b = static_cast<float>(((i >> 8) & 0xFF)) / MAX_COLOR_VALUE;
            a = static_cast<float>((i & 0xFF)) / MAX_COLOR_VALUE;
        }
        catch (std::invalid_argument const&)
        {
            context.builder << "Invalid argument for hex value";
            create_error(node, context);
            return false;
        }
    }

    r = std::clamp(r, 0.f, 1.f);
    g = std::clamp(g, 0.f, 1.f);
    b = std::clamp(b, 0.f, 1.f);
    a = std::clamp(a, 0.f, 1.f);

    color = { r, g, b, a };
    return true;
}

bool try_parse_color(YAML::Node const& root, const char* key, glm::vec4& color, ParsingContext& context)
{
    if (!root[key])
    {
        context.builder << "Node is missing key: " << key;
        create_error(root, context);
        return false;
    }

    return try_parse_color(root[key], color, context);
}

void read_includes(YAML::Node const& node, ParsingContext& context)
{
    if (!node.IsSequence())
    {
        context.builder << "Expected list of includes";
        create_error(node, context);
        return;
    }

    std::vector<std::string> includes;
    for (auto const& include_node : node)
    {
        if (!include_node.IsScalar())
        {
            context.builder << "Expected a string";
            create_error(include_node, context);
            return;
        }

        includes.push_back(include_node.as<std::string>());
    }

    context.result.config.includes = std::move(includes);
}

void read_plugins(YAML::Node const& node, ParsingContext& context)
{
    if (!node.IsSequence())
    {
        context.builder << "Expected list of plugins";
        create_error(node, context);
        return;
    }

    std::vector<miracle::PluginConfiguration> plugins;
    for (auto const& plugin_node : node)
    {
        std::string path;
        if (!try_parse_value(plugin_node, "path", path, context))
            return;

        std::string name;
        if (!try_parse_value(plugin_node, "name", name, context))
            return;
        miracle::PluginConfiguration plugin_config;
        plugin_config.path = path;
        plugin_config.name = name;

        std::string func_name;
        if (try_parse_value(plugin_node, "add_points", func_name, context, true))
            plugin_config.add_points_function = func_name;
        if (try_parse_value(plugin_node, "animate", func_name, context, true))
            plugin_config.animate_function = func_name;
        if (try_parse_value(plugin_node, "place_new_window", func_name, context, true))
            plugin_config.place_new_window_function = func_name;

        plugins.push_back(plugin_config);
    }
    context.result.config.plugins = std::move(plugins);
}

void read_action_key(YAML::Node const& node, ParsingContext& context)
{
    if (auto const modifier = try_parse_string_to_optional_value<std::optional<uint>>(node, try_parse_modifier, context))
        context.result.config.primary_modifier = modifier.value();
}

void read_default_action_overrides(YAML::Node const& default_action_overrides, ParsingContext& context)
{
    if (!default_action_overrides.IsSequence())
    {
        context.builder << "Default action overrides must be an array";
        create_error(default_action_overrides, context);
        return;
    }

    for (auto const& sub_node : default_action_overrides)
    {
        std::string name;
        if (!try_parse_value(sub_node, "name", name, context))
            continue;

        std::string action;
        if (!try_parse_value(sub_node, "action", action, context))
            continue;

        std::string key;
        if (!try_parse_value(sub_node, "key", key, context))
            return;

        auto const& modifiers_node = sub_node["modifiers"];

        auto key_command = miracle::DefaultKeyCommand::MAX;
        for (size_t i = 0; i < miracle::default_key_command_strings.size(); i++)
        {
            if (miracle::default_key_command_strings[i] == name)
                key_command = static_cast<miracle::DefaultKeyCommand>(i);
        }

        if (key_command == miracle::DefaultKeyCommand::MAX)
        {
            context.builder << "Unknown key command override: " << sub_node["name"];
            create_error(sub_node["name"], context);
            continue;
        }

        auto keyboard_action = try_parse_string_to_optional_value<std::optional<MirKeyboardAction>>(sub_node, "action", from_string_keyboard_action, context);
        if (!keyboard_action)
            continue;

        auto const code = libevdev_event_code_from_name(EV_KEY, key.c_str()); // https://stackoverflow.com/questions/32059363/is-there-a-way-to-get-the-evdev-keycode-from-a-string
        if (code < 0)
        {
            context.builder << "Unknown keyboard code in configuration: " << key.c_str() << ". See the linux kernel for allowed codes: https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h";
            create_error(sub_node["key"], context);
            continue;
        }

        uint modifiers = 0;
        if (!try_parse_modifiers(modifiers_node, modifiers, context))
            continue;

        context.result.config.built_in_key_command_overrides->push_back({ keyboard_action.value(),
            modifiers,
            static_cast<uint>(code),
            key_command });
    }
}

void read_custom_actions(YAML::Node const& custom_actions, ParsingContext& context)
{
    if (!custom_actions.IsSequence())
    {
        context.builder << "Custom actions must be an array";
        create_error(custom_actions, context);
        return;
    }

    for (auto const& sub_node : custom_actions)
    {
        std::string command;
        std::string key;
        if (!try_parse_value(sub_node, "command", command, context))
            continue;
        auto keyboard_action = try_parse_string_to_optional_value<std::optional<MirKeyboardAction>>(sub_node, "action", from_string_keyboard_action, context);
        if (!keyboard_action)
            continue;
        if (!try_parse_value(sub_node, "key", key, context))
            continue;

        auto const code = libevdev_event_code_from_name(EV_KEY,
            key.c_str()); // https://stackoverflow.com/questions/32059363/is-there-a-way-to-get-the-evdev-keycode-from-a-string
        if (code < 0)
        {
            context.builder << "Unknown keyboard code in configuration: " << key.c_str() << ". See the linux kernel for allowed codes: https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h";
            create_error(sub_node["key"], context);
            continue;
        }

        YAML::Node modifiers_node = sub_node["modifiers"];
        if (!modifiers_node)
        {
            context.builder << "Missing 'modifiers' in item";
            create_error(sub_node, context);
            continue;
        }

        uint modifiers = 0;
        if (!try_parse_modifiers(modifiers_node, modifiers, context))
            continue;

        context.result.config.custom_key_commands->push_back({ keyboard_action.value(),
            modifiers,
            static_cast<uint>(code),
            command });
    }
}

void read_inner_gaps(YAML::Node const& node, ParsingContext& context)
{
    uint x = 0, y = 0;
    if (!try_parse_value(node, "x", x, context))
        return;
    if (!try_parse_value(node, "y", y, context))
        return;

    miracle::Gaps inner_gaps;
    inner_gaps.top = y;
    inner_gaps.bottom = y;
    inner_gaps.left = x;
    inner_gaps.right = x;
    context.result.config.inner_gaps = inner_gaps;
}

void read_outer_gaps(YAML::Node const& node, ParsingContext& context)
{
    uint x = 0, y = 0;
    if (!try_parse_value(node, "x", x, context))
        return;
    if (!try_parse_value(node, "y", y, context))
        return;

    miracle::Gaps outer_gaps;
    outer_gaps.top = y;
    outer_gaps.bottom = y;
    outer_gaps.left = x;
    outer_gaps.right = x;
    context.result.config.outer_gaps = outer_gaps;
}

void read_startup_apps(YAML::Node const& startup_apps, ParsingContext& context)
{
    if (!startup_apps.IsSequence())
    {
        context.builder << "Expected startup applications to be an array";
        create_error(startup_apps, context);
        return;
    }

    std::vector<miracle::StartupApp> startup_apps_vec;
    for (auto const& node : startup_apps)
    {
        std::string command;
        if (!try_parse_value(node, "command", command, context))
            continue;

        bool restart_on_death = false;
        if (node["restart_on_death"])
        {
            if (!try_parse_value(node, "restart_on_death", restart_on_death, context))
                continue;
        }

        bool in_systemd_scope = false;
        if (node["in_systemd_scope"])
        {
            if (!try_parse_value(node, "in_systemd_scope", in_systemd_scope, context))
                continue;
        }

        bool no_startup_id = false;
        if (node["no_startup_id"])
        {
            if (!try_parse_value(node, "no_startup_id", in_systemd_scope, context))
                continue;
        }

        bool should_halt_compositor_on_death = false;
        if (node["should_halt_compositor_on_death"])
        {
            if (!try_parse_value(node, "should_halt_compositor_on_death", in_systemd_scope, context))
                continue;
        }

        startup_apps_vec.push_back({ .command = std::move(command),
            .restart_on_death = restart_on_death,
            .no_startup_id = no_startup_id,
            .should_halt_compositor_on_death = should_halt_compositor_on_death,
            .in_systemd_scope = in_systemd_scope });
    }

    context.result.config.startup_apps = startup_apps_vec;
}

void read_terminal(YAML::Node const& node, ParsingContext& context)
{
    std::string desired_terminal;
    if (!try_parse_value(node, desired_terminal, context))
        return;

    if (!program_exists(desired_terminal))
    {
        context.builder << "Cannot find requested terminal program: " << desired_terminal;
        create_error(node, context);
        return;
    }

    context.result.config.terminal = desired_terminal;
}

void read_resize_jump(YAML::Node const& node, ParsingContext& context)
{
    int resize_jump;
    if (try_parse_value(node, resize_jump, context))
        context.result.config.resize_jump = resize_jump;
}

void read_environment_variables(YAML::Node const& env, ParsingContext& context)
{
    if (!env.IsSequence())
    {
        context.builder << "Expected environment variables to be an array";
        create_error(env, context);
        return;
    }

    std::vector<miracle::EnvironmentVariable> variables;
    for (auto const& node : env)
    {
        std::string key, value;
        if (!try_parse_value(node, "key", key, context))
            continue;
        if (!try_parse_value(node, "value", value, context))
            continue;
        variables.push_back({ key, value });
    }

    context.result.config.environment_variables = variables;
}

void read_border(YAML::Node const& border, ParsingContext& context)
{
    int size = 0;
    try_parse_value(border, "size", size, context, true);

    float radius = 8.f;
    try_parse_value(border, "radius", radius, context, true);

    glm::vec4 color = glm::vec4(0);
    try_parse_color(border, "color", color, context);
    glm::vec4 focus_color = glm::vec4(0);
    try_parse_color(border, "focus_color", focus_color, context);
    context.result.config.border_config = { size, radius, focus_color, color };
}

void read_workspaces(YAML::Node const& workspaces, ParsingContext& context)
{
    if (!workspaces.IsSequence())
    {
        context.builder << "Expected workspaces to be a sequence";
        create_error(workspaces, context);
        return;
    }

    std::vector<miracle::WorkspaceConfig> workspace_configs;
    for (auto const& workspace : workspaces)
    {
        int num;
        std::string name;
        auto const has_number = try_parse_value(workspace, "number", num, context, true);
        auto const has_name = try_parse_value(workspace, "name", name, context, true);
        if (!has_name && !has_number)
        {
            context.builder << "Workspace configuration must include either a 'name' or a 'number' key";
            create_error(workspace, context);
            continue;
        }

        workspace_configs.push_back({ num,
            name.empty() ? std::optional<std::string>(std::nullopt) : name });
    }

    context.result.config.workspace_configs = workspace_configs;
}

namespace
{
    bool try_read_built_in_animation_definition(YAML::Node const& node, ParsingContext& context, miracle::BuiltInAnimationDefinition& animation_def)
    {
        auto const& type = try_parse_string_to_optional_value<std::optional<miracle::BultInAnimationType>>(
            node,
            "type",
            from_string_built_in_animation_type,
            context);
        if (!type)
            return false;

        auto const& function = try_parse_string_to_optional_value<std::optional<miracle::EaseFunction>>(
            node,
            "function",
            from_string_ease_function,
            context);
        if (!function)
            return false;

        animation_def.type = type.value();
        animation_def.function = function.value();
        try_parse_value(node, "c1", animation_def.c1, context, true);
        try_parse_value(node, "c2", animation_def.c2, context, true);
        try_parse_value(node, "c3", animation_def.c3, context, true);
        try_parse_value(node, "c4", animation_def.c4, context, true);
        try_parse_value(node, "n1", animation_def.n1, context, true);
        try_parse_value(node, "d1", animation_def.d1, context, true);
        return true;
    }
}

void read_animation_definitions(YAML::Node const& animation_node_list, ParsingContext& context)
{
    if (!animation_node_list.IsSequence())
    {
        context.builder << "Animation definitions must be a sequence";
        create_error(animation_node_list, context);
        return;
    }

    for (auto const& animation_node : animation_node_list)
    {
        auto const event = try_parse_string_to_optional_value<std::optional<miracle::AnimateableEvent>>(
            animation_node,
            "event",
            from_string_animateable_event,
            context);
        if (!event)
        {
            context.builder << "Animation definition is missing or has invalid 'event' key";
            create_error(animation_node, context);
            continue;
        }

        auto const event_as_int = static_cast<size_t>(event.value());
        auto const type = try_parse_string_to_optional_value<std::optional<miracle::AnimationType>>(
            animation_node,
            "type",
            from_string_animation_type,
            context);
        if (!type)
        {
            context.builder << "Animation definition is missing or has invalid 'type' key";
            create_error(animation_node, context);
            continue;
        }

        miracle::AnimationDefinition definition;
        definition.type = type.value();
        bool success = false;
        switch (type.value())
        {
        case miracle::AnimationType::built_in:
        {
            miracle::BuiltInAnimationList animations;
            if (!animation_node["parts"].IsSequence())
            {
                context.builder << "Built-in animation definitions must have an 'animation_list' key with a list of animations";
                create_error(animation_node, context);
                break;
            }

            for (auto const built_in_animation_node : animation_node["parts"])
            {
                miracle::BuiltInAnimationDefinition animation_def;
                if (try_read_built_in_animation_definition(built_in_animation_node, context, animation_def))
                    animations.push_back(animation_def);
            }

            definition.data = animations;
            success = true;
            break;
        }
        case miracle::AnimationType::plugin:
        {
            if (!animation_node["plugin_name"])
            {
                context.builder << "Plugin animation definitions must have a 'plugin_name' key";
                create_error(animation_node, context);
                break;
            }
            std::string plugin_name;
            if (!try_parse_value(animation_node, "plugin_name", plugin_name, context))
            {
                context.builder << "Plugin animation definitions must have a valid 'plugin_name' key";
                create_error(animation_node, context);
                break;
            }

            definition.data = miracle::PluginAnimationDefinition { plugin_name };
            success = true;
            break;
        }
        default:
            context.builder << "Unsupported animation type in definition";
            create_error(animation_node, context);
            break;
        }

        if (success)
        {
            definition.is_default = false;
            // Parse the optional 'duration' value
            try_parse_value(animation_node, "duration", definition.duration_seconds, context, true);
            context.result.config.animation_definitions.value[event_as_int] = definition;
        }
    }
}

void read_enable_animations(YAML::Node const& node, ParsingContext& context)
{
    bool animations_enabled;
    if (try_parse_value(node, animations_enabled, context))
        context.result.config.animations_enabled = animations_enabled;
}

void read_move_modifier(YAML::Node const& node, ParsingContext& context)
{
    uint move_modifier;
    if (try_parse_modifiers(node, move_modifier, context))
        context.result.config.move_modifier = move_modifier;
}

void read_drag_and_drop(YAML::Node const& node, ParsingContext& context)
{
    miracle::DragAndDropConfiguration drag_and_drop;
    try_parse_value(node, "enabled", drag_and_drop.enabled, context, true);
    uint modifiers = 0;
    if (node["modifiers"])
    {
        if (!try_parse_modifiers(node["modifiers"], modifiers, context))
            return;

        drag_and_drop.modifiers = modifiers;
    }

    context.result.config.drag_and_drop = drag_and_drop;
}

void read_mouse(YAML::Node const& node, ParsingContext& context)
{
    miral::InputConfiguration::Mouse mouse_configuration;
    auto const handedness = try_parse_string_to_optional_value<std::optional<MirPointerHandedness>>(
        node,
        "handedness",
        from_string_handedness,
        context);
    mouse_configuration.handedness(handedness);

    double vscroll_speed;
    if (try_parse_value(node, "vscroll_speed", vscroll_speed, context, true))
        mouse_configuration.vscroll_speed(vscroll_speed);

    double hscroll_speed;
    if (try_parse_value(node, "hscroll_speed", hscroll_speed, context, true))
        mouse_configuration.hscroll_speed(hscroll_speed);

    double acceleration_bias;
    if (try_parse_value(node, "acceleration_bias", acceleration_bias, context, true))
    {
        acceleration_bias = std::clamp(acceleration_bias, -1.0, 1.0);
        mouse_configuration.acceleration_bias(acceleration_bias);
    }

    auto const acceleration = try_parse_string_to_optional_value<std::optional<MirPointerAcceleration>>(
        node,
        "acceleration",
        from_string_acceleration,
        context);
    mouse_configuration.acceleration(acceleration);

    context.result.config.mouse_configuration = mouse_configuration;
}

void read_touchpad(YAML::Node const& node, ParsingContext& context)
{
    miracle::TouchpadConfiguration touchpad_config;

    try_parse_value(node, "disable_while_typing", touchpad_config.disable_while_typing, context, true);
    try_parse_value(node, "disable_with_external_mouse", touchpad_config.disable_with_external_mouse, context, true);
    try_parse_value(node, "tap_to_click", touchpad_config.tap_to_click, context, true);
    try_parse_value(node, "middle_mouse_button_emulation", touchpad_config.middle_mouse_button_emulation, context, true);

    double vscroll_speed;
    if (try_parse_value(node, "vscroll_speed", vscroll_speed, context, true))
        touchpad_config.vscroll_speed = static_cast<float>(vscroll_speed);

    double hscroll_speed;
    if (try_parse_value(node, "hscroll_speed", hscroll_speed, context, true))
        touchpad_config.hscroll_speed = static_cast<float>(hscroll_speed);

    double acceleration_bias;
    if (try_parse_value(node, "acceleration_bias", acceleration_bias, context, true))
    {
        acceleration_bias = std::clamp(acceleration_bias, -1.0, 1.0);
        touchpad_config.acceleration_bias = static_cast<float>(acceleration_bias);
    }

    auto const click_mode = try_parse_string_to_optional_value<std::optional<MirTouchpadClickMode>>(
        node,
        "click_mode",
        from_string_touchpad_click_mode,
        context);
    if (click_mode.has_value())
        touchpad_config.click_mode = click_mode.value();

    auto const scroll_mode = try_parse_string_to_optional_value<std::optional<MirTouchpadScrollMode>>(
        node,
        "scroll_mode",
        from_string_touchpad_scroll_mode,
        context);
    if (scroll_mode.has_value())
        touchpad_config.scroll_mode = scroll_mode.value();

    context.result.config.touchpad = touchpad_config;
}

void read_keyboard(YAML::Node const& node, ParsingContext& context)
{
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
    miral::InputConfiguration::Keyboard keyboard_config;
    int repeat_delay = 0;
    int repeat_rate = 0;
    if (try_parse_value(node, "repeat_delay", repeat_delay, context, true))
        keyboard_config.set_repeat_delay(repeat_delay);

    if (try_parse_value(node, "repeat_rate", repeat_rate, context, true))
        keyboard_config.set_repeat_rate(repeat_rate);

    context.result.config.keyboard_configuration = keyboard_config;
#endif

    if (node["keymap"])
    {
        miracle::KeymapConfiguration keymap;
        auto const& keymap_node = node["keymap"];
        std::string language;
        if (!try_parse_value(keymap_node, "language", language, context))
        {
            context.builder << "Expected 'language' to be provided under the 'keymap' key";
            create_error(keymap_node, context);
        }
        else
        {
            context.result.config.keymap = miracle::KeymapConfiguration();
            keymap.language = language;
            std::string variant;
            if (try_parse_value(keymap_node, "variant", variant, context))
                keymap.variant = variant;
            else
                keymap.variant = std::nullopt;

            keymap.options = {};
            if (keymap_node["options"])
            {
                if (!keymap_node["options"].IsSequence())
                {
                    context.builder << "Expected 'options' to be a sequence";
                    create_error(keymap_node["options"], context);
                }
                else
                {
                    for (auto const& option : keymap_node["options"])
                    {
                        std::string name;
                        if (!try_parse_value(option, name, context))
                            continue;

                        keymap.options.emplace_back(name);
                    }
                }
            }
        }

        context.result.config.keymap = keymap;
    }
}

void read_hover_click(YAML::Node const& node, ParsingContext& context)
{
    miracle::HoverClickConfiguration hover_click;
    try_parse_value(node, "enabled", hover_click.enabled, context, true);
    try_parse_value(node, "hover_duration", hover_click.hover_duration_milliseconds, context, true);
    try_parse_value(node, "cancel_displacement_threshold", hover_click.cancel_displacement_threshold, context, true);
    try_parse_value(node, "reclick_displacement_threshold", hover_click.reclick_displacement_threshold, context, true);

    context.result.config.hover_click = hover_click;
}

void read_simulated_secondary_click(YAML::Node const& node, ParsingContext& context)
{
    miracle::SimulatedSecondaryClickConfiguration simulated_secondary_click;
    try_parse_value(node, "enabled", simulated_secondary_click.enabled, context, true);
    try_parse_value(node, "hold_duration", simulated_secondary_click.hold_duration_milliseconds, context, true);
    try_parse_value(node, "displacement_threshold", simulated_secondary_click.displacement_threshold, context, true);

    context.result.config.simulated_secondary_click = simulated_secondary_click;
}

void read_output_filter(YAML::Node const& node, ParsingContext& context)
{
    miracle::OutputFilterConfiguration output_filter;
    std::string shader_path;
    if (try_parse_value(node, "shader_path", shader_path, context, true))
        output_filter.shader_path = shader_path;
    else
        output_filter.shader_path = std::nullopt;

    context.result.config.output_filter = output_filter;
}

void read_cursor(YAML::Node const& node, ParsingContext& context)
{
    miracle::CursorConfiguration cursor;
    try_parse_value(node, "scale", cursor.scale, context, true);
    if (auto mode = try_parse_string_to_optional_value<std::optional<miracle::CursorFocusMode>>(node, "focus_mode", from_string_cursor_focus_mode, context))
        cursor.focus_mode = mode.value();

    context.result.config.cursor = cursor;
}

void read_slow_keys(YAML::Node const& node, ParsingContext& context)
{
    miracle::SlowKeysConfiguration slow_keys;
    try_parse_value(node, "enabled", slow_keys.enabled, context, true);
    try_parse_value(node, "hold_delay", slow_keys.hold_delay_milliseconds, context, true);
    context.result.config.slow_keys = slow_keys;
}

void read_sticky_keys(YAML::Node const& node, ParsingContext& context)
{
    miracle::StickyKeysConfiguration sticky_keys;
    try_parse_value(node, "enabled", sticky_keys.enabled, context, true);
    try_parse_value(node, "should_disable_if_two_keys_are_pressed_together", sticky_keys.should_disable_if_two_keys_are_pressed_together, context, true);
    context.result.config.sticky_keys = sticky_keys;
}

void read_magnifier(YAML::Node const& node, ParsingContext& context)
{
    miracle::MagnifierConfiguration magnifier;
    try_parse_value(node, "enabled", magnifier.enabled, context, true);
    try_parse_value(node, "scale", magnifier.scale, context, true);
    try_parse_value(node, "scale_increment", magnifier.scale_increment, context, true);
    try_parse_value(node, "width", magnifier.width, context, true);
    try_parse_value(node, "height", magnifier.height, context, true);
    try_parse_value(node, "size_increment", magnifier.size_increment, context, true);
    context.result.config.magnifier = magnifier;
}

void read_workspace_back_and_forth(YAML::Node const& node, ParsingContext& context)
{
    bool workspace_back_and_forth;
    if (try_parse_value(node, workspace_back_and_forth, context))
        context.result.config.workspace_back_and_forth = workspace_back_and_forth;
}
}

miracle::ConfigData::ConfigData() :
    animation_definitions { default_animation_definitions }
{
}

miracle::ConfigLoadResult miracle::load_config(std::string const& path)
{
    ParsingContext context;

    try
    {
        YAML::Node config = YAML::LoadFile(path);
        if (config["includes"])
            read_includes(config["includes"], context);
        if (config["plugins"])
            read_plugins(config["plugins"], context);
        if (config["action_key"])
            read_action_key(config["action_key"], context);
        if (config["default_action_overrides"])
            read_default_action_overrides(config["default_action_overrides"], context);
        if (config["custom_actions"])
            read_custom_actions(config["custom_actions"], context);
        if (config["inner_gaps"])
            read_inner_gaps(config["inner_gaps"], context);
        if (config["outer_gaps"])
            read_outer_gaps(config["outer_gaps"], context);
        if (config["startup_apps"])
            read_startup_apps(config["startup_apps"], context);
        if (config["terminal"])
            read_terminal(config["terminal"], context);
        if (config["resize_jump"])
            read_resize_jump(config["resize_jump"], context);
        if (config["environment_variables"])
            read_environment_variables(config["environment_variables"], context);
        if (config["border"])
            read_border(config["border"], context);
        if (config["workspaces"])
            read_workspaces(config["workspaces"], context);
        if (config["animations"])
            read_animation_definitions(config["animations"], context);
        if (config["enable_animations"])
            read_enable_animations(config["enable_animations"], context);
        if (config["move_modifier"])
            read_move_modifier(config["move_modifier"], context);
        if (config["drag_and_drop"])
            read_drag_and_drop(config["drag_and_drop"], context);
        if (config["mouse"])
            read_mouse(config["mouse"], context);
        if (config["touchpad"])
            read_touchpad(config["touchpad"], context);
        if (config["keyboard"])
            read_keyboard(config["keyboard"], context);
        if (config["hover_click"])
            read_hover_click(config["hover_click"], context);
        if (config["simulated_secondary_click"])
            read_simulated_secondary_click(config["simulated_secondary_click"], context);
        if (config["output_filter"])
            read_output_filter(config["output_filter"], context);
        if (config["cursor"])
            read_cursor(config["cursor"], context);
        if (config["slow_keys"])
            read_slow_keys(config["slow_keys"], context);
        if (config["sticky_keys"])
            read_sticky_keys(config["sticky_keys"], context);
        if (config["magnifier"])
            read_magnifier(config["magnifier"], context);
        if (config["workspace_back_and_forth"])
            read_workspace_back_and_forth(config["workspace_back_and_forth"], context);
    }
    catch (YAML::Exception const& e)
    {
        context.builder << "Encountered exception during config load: " << e.what();
        context.result.errors.push_back({ e.mark.line,
            e.mark.column,
            ErrorLevel::error,
            path,
            context.builder.str() });
    }
    catch (const std::exception& e)
    {
        context.builder << "Encountered exception during config load: " << e.what();
        context.result.errors.push_back({ 0,
            0,
            ErrorLevel::error,
            path,
            context.builder.str() });
    }
    catch (...)
    {
        context.builder << "Encountered an unknown exception";
        context.result.errors.push_back({ 0,
            0,
            ErrorLevel::error,
            path,
            context.builder.str() });
    }

    return context.result;
}

miracle::ConfigSaveResult miracle::save_config(std::string const& path, ConfigData const& config)
{
    ConfigSaveResult result(true, {});
    YAML::Emitter out;
    out << YAML::BeginMap;

    if (config.includes.is_set())
    {
        out << YAML::Key << "includes" << YAML::Value << YAML::BeginSeq;

        for (auto const& include : *config.includes)
            out << include;

        out << YAML::EndSeq;
    }

    // Save plugins
    if (config.plugins.is_set())
    {
        out << YAML::Key << "plugins" << YAML::Value << YAML::BeginSeq;
        for (auto const& plugin : *config.plugins)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "path" << YAML::Value << plugin.path;
            out << YAML::Key << "name" << YAML::Value << plugin.name;
            if (plugin.add_points_function)
                out << YAML::Key << "add_points" << YAML::Value << *plugin.add_points_function;
            if (plugin.animate_function)
                out << YAML::Key << "animate" << YAML::Value << *plugin.animate_function;
            if (plugin.place_new_window_function)
                out << YAML::Key << "place_new_window" << YAML::Value << *plugin.place_new_window_function;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save primary modifier
    for (auto const& [name, value] : mir_input_event_modifier_opts)
    {
        if (value == config.primary_modifier.value)
        {
            out << YAML::Key << "action_key" << YAML::Value << name;
            break;
        }
    }

    // Save default action overrides
    if (!config.built_in_key_command_overrides.is_default_value)
    {
        out << YAML::Key << "default_action_overrides" << YAML::Value << YAML::BeginSeq;
        for (auto const& override : *config.built_in_key_command_overrides)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << default_key_command_strings[static_cast<uint32_t>(override.default_key_command)];
            out << YAML::Key << "action" << YAML::Value << mir_keyboard_actions_strings[override.action].first;
            out << YAML::Key << "key" << YAML::Value << libevdev_event_code_get_name(EV_KEY, static_cast<uint32_t>(override.key));

            out << YAML::Key << "modifiers" << YAML::Value << YAML::BeginSeq;
            for (auto const& [name, value] : mir_input_event_modifier_opts)
            {
                if (override.modifiers & value)
                    out << name;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save custom actions
    if (!config.custom_key_commands.is_default_value)
    {
        out << YAML::Key << "custom_actions" << YAML::Value << YAML::BeginSeq;
        for (auto const& action : *config.custom_key_commands)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "command" << YAML::Value << action.command;
            out << YAML::Key << "action" << YAML::Value << mir_keyboard_actions_strings[action.action].first;
            out << YAML::Key << "key" << YAML::Value << libevdev_event_code_get_name(EV_KEY, static_cast<uint32_t>(action.key));

            out << YAML::Key << "modifiers" << YAML::Value << YAML::BeginSeq;
            for (auto const& [name, value] : mir_input_event_modifier_opts)
            {
                if (action.modifiers & value)
                    out << name;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save gaps
    if (!config.inner_gaps.is_default_value)
    {

        out << YAML::Key << "inner_gaps" << YAML::Value << YAML::BeginMap
            << YAML::Key << "x" << YAML::Value << config.inner_gaps->left
            << YAML::Key << "y" << YAML::Value << config.inner_gaps->top
            << YAML::EndMap;
    }

    if (!config.outer_gaps.is_default_value)
    {
        out << YAML::Key << "outer_gaps" << YAML::Value << YAML::BeginMap
            << YAML::Key << "x" << YAML::Value << config.outer_gaps->left
            << YAML::Key << "y" << YAML::Value << config.outer_gaps->top
            << YAML::EndMap;
    }

    // Save startup apps
    if (!config.startup_apps.is_default_value)
    {
        out << YAML::Key << "startup_apps" << YAML::Value << YAML::BeginSeq;
        for (auto const& app : *config.startup_apps)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "command" << YAML::Value << app.command;
            if (app.restart_on_death)
                out << YAML::Key << "restart_on_death" << YAML::Value << app.restart_on_death;
            if (app.no_startup_id)
                out << YAML::Key << "no_startup_id" << YAML::Value << app.no_startup_id;
            if (app.should_halt_compositor_on_death)
                out << YAML::Key << "should_halt_compositor_on_death" << YAML::Value << app.should_halt_compositor_on_death;
            if (app.in_systemd_scope)
                out << YAML::Key << "in_systemd_scope" << YAML::Value << app.in_systemd_scope;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save terminal
    if (!config.terminal.is_default_value && *config.terminal)
        out << YAML::Key << "terminal" << YAML::Value << *config.terminal.value;

    // Save resize jump
    out << YAML::Key << "resize_jump" << YAML::Value << config.resize_jump;

    // Save environment variables
    if (!config.environment_variables.is_default_value)
    {
        out << YAML::Key << "environment_variables" << YAML::Value << YAML::BeginSeq;
        for (auto const& var : *config.environment_variables)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "key" << YAML::Value << var.key;
            out << YAML::Key << "value" << YAML::Value << var.value;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save border config
    if (!config.border_config.is_default_value)
    {
        out << YAML::Key << "border" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "size" << YAML::Value << config.border_config->size;
        out << YAML::Key << "radius" << YAML::Value << config.border_config->radius;

        // Save colors as hex values
        auto to_hex = [](glm::vec4 const& color)
        {
            return (static_cast<int>(color.r * 255) << 24) | (static_cast<int>(color.g * 255) << 16) | (static_cast<int>(color.b * 255) << 8) | (static_cast<int>(color.a * 255));
        };

        out << YAML::Key << "color" << YAML::Value << YAML::Hex << to_hex(config.border_config->color);
        out << YAML::Key << "focus_color" << YAML::Value << YAML::Hex << to_hex(config.border_config->focus_color);
        out << YAML::EndMap;
    }

    // Save workspaces
    if (!config.workspace_configs.is_default_value)
    {
        out << YAML::Key << "workspaces" << YAML::Value << YAML::BeginSeq;
        for (auto const& workspace : *config.workspace_configs)
        {
            if (!workspace.num && !workspace.name)
                continue;

            out << YAML::BeginMap;
            if (workspace.num)
                out << YAML::Key << "number" << YAML::Value << workspace.num.value();
            if (workspace.name)
                out << YAML::Key << "name" << YAML::Value << workspace.name.value();
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save animations
    if (!config.animations_enabled)
    {
        out << YAML::Key << "enable_animations" << YAML::Value << config.animations_enabled;
    }

    // Save animation definitions
    bool has_custom_animations = false;
    for (auto const& def : *config.animation_definitions)
    {
        if (!def.is_default)
        {
            has_custom_animations = true;
            break;
        }
    }

    if (has_custom_animations)
    {
        out << YAML::Key << "animations" << YAML::Value << YAML::BeginSeq;
        for (size_t i = 0; i < static_cast<size_t>(AnimateableEvent::max); i++)
        {
            auto const& def = config.animation_definitions.value[i];
            if (def.is_default)
                continue;

            out << YAML::BeginMap;
            out << YAML::Key << "event" << YAML::Value << animateable_event_strings[i];
            if (def.duration_seconds != 0.f)
                out << YAML::Key << "duration" << YAML::Value << def.duration_seconds;
            out << YAML::Key << "type" << YAML::Value << animation_type_strings[static_cast<uint32_t>(def.type)];

            switch (def.type)
            {
            case AnimationType::built_in:
                out << YAML::Key << "parts" << YAML::Value << YAML::BeginSeq;
                for (auto const& animation : std::get<BuiltInAnimationList>(def.data))
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "type" << YAML::Value << built_in_animation_type_strings[static_cast<uint32_t>(animation.type)];
                    out << YAML::Key << "function" << YAML::Value << ease_function_strings[static_cast<uint32_t>(animation.function)];
                    if (animation.c1 != 0.f)
                        out << YAML::Key << "c1" << YAML::Value << animation.c1;
                    if (animation.c2 != 0.f)
                        out << YAML::Key << "c2" << YAML::Value << animation.c2;
                    if (animation.c3 != 0.f)
                        out << YAML::Key << "c3" << YAML::Value << animation.c3;
                    if (animation.c4 != 0.f)
                        out << YAML::Key << "c4" << YAML::Value << animation.c4;
                    if (animation.n1 != 0.f)
                        out << YAML::Key << "n1" << YAML::Value << animation.n1;
                    if (animation.d1 != 0.f)
                        out << YAML::Key << "d1" << YAML::Value << animation.d1;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
                break;
            case AnimationType::plugin:
                out << YAML::Key << "plugin_name" << YAML::Value << std::get<PluginAnimationDefinition>(def.data).plugin_name;
                break;
            default:
                break;
            }
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save move modifier
    if (!config.move_modifier.is_default_value)
    {
        out << YAML::Key << "move_modifier" << YAML::Value << YAML::BeginSeq;
        for (auto const& [name, value] : mir_input_event_modifier_opts)
        {
            if (config.move_modifier & value)
                out << name;
        }
        out << YAML::EndSeq;
    }

    // Save drag and drop
    if (!config.drag_and_drop.is_default_value)
    {
        out << YAML::Key << "drag_and_drop" << YAML::Value << YAML::BeginMap;
        if (!config.drag_and_drop->enabled)
            out << YAML::Key << "enabled" << YAML::Value << config.drag_and_drop->enabled;

        if (config.drag_and_drop->modifiers != (miracle_input_event_modifier_default | mir_input_event_modifier_shift))
        {
            out << YAML::Key << "modifiers" << YAML::Value << YAML::BeginSeq;
            for (auto const& [name, value] : mir_input_event_modifier_opts)
            {
                if (config.drag_and_drop->modifiers & value)
                    out << name;
            }
            out << YAML::EndSeq;
        }
        out << YAML::EndMap;
    }

    // Save mouse config only if we're different from the empty mouse config
    if (!config.mouse_configuration.is_default_value)
    {
        out << YAML::Key << "mouse" << YAML::Value << YAML::BeginMap;

        if (config.mouse_configuration->handedness() != std::nullopt)
            out << YAML::Key << "handedness" << YAML::Value << to_string_handedness(config.mouse_configuration->handedness().value());
        if (config.mouse_configuration->vscroll_speed() != std::nullopt)
            out << YAML::Key << "vscroll_speed" << YAML::Value << config.mouse_configuration->vscroll_speed().value();
        if (config.mouse_configuration->hscroll_speed() != std::nullopt)
            out << YAML::Key << "hscroll_speed" << YAML::Value << config.mouse_configuration->hscroll_speed().value();
        if (config.mouse_configuration->acceleration_bias() != std::nullopt)
            out << YAML::Key << "acceleration_bias" << YAML::Value << config.mouse_configuration->acceleration_bias().value();
        if (config.mouse_configuration->acceleration() != std::nullopt)
            out << YAML::Key << "acceleration" << YAML::Value << to_string_acceleration(config.mouse_configuration->acceleration().value());

        out << YAML::EndMap;
    }

    // Save touchpad config only if we're different from the default touchpad config
    if (!config.touchpad.is_default_value)
    {
        out << YAML::Key << "touchpad" << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "disable_while_typing" << YAML::Value << config.touchpad->disable_while_typing;
        out << YAML::Key << "disable_with_external_mouse" << YAML::Value << config.touchpad->disable_with_external_mouse;
        out << YAML::Key << "tap_to_click" << YAML::Value << config.touchpad->tap_to_click;
        out << YAML::Key << "middle_mouse_button_emulation" << YAML::Value << config.touchpad->middle_mouse_button_emulation;
        out << YAML::Key << "acceleration_bias" << YAML::Value << config.touchpad->acceleration_bias;
        out << YAML::Key << "vscroll_speed" << YAML::Value << config.touchpad->vscroll_speed;
        out << YAML::Key << "hscroll_speed" << YAML::Value << config.touchpad->hscroll_speed;
        out << YAML::Key << "click_mode" << YAML::Value << to_string_touchpad_click_mode(config.touchpad->click_mode);
        out << YAML::Key << "scroll_mode" << YAML::Value << to_string_touchpad_scroll_mode(config.touchpad->scroll_mode);

        out << YAML::EndMap;
    }

    if (!config.keymap.is_default_value)
    {
        out << YAML::Key << "keyboard" << YAML::Value << YAML::BeginMap;
        if (config.keyboard_configuration->repeat_delay())
            out << YAML::Key << "repeat_delay" << YAML::Value << *config.keyboard_configuration->repeat_delay();

        if (config.keyboard_configuration->repeat_rate())
            out << YAML::Key << "repeat_rate" << YAML::Value << *config.keyboard_configuration->repeat_rate();

        if (!config.keymap.is_default_value)
        {
            out << YAML::Key << "keymap" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "language" << YAML::Value << config.keymap->value().language;
            if (config.keymap->value().variant)
                out << YAML::Key << "variant" << YAML::Value << *config.keymap->value().variant;
            out << YAML::Key << "options" << YAML::Value << YAML::BeginSeq;
            for (auto const& option : config.keymap->value().options)
                out << option;

            out << YAML::EndSeq;
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    if (!config.hover_click.is_default_value)
    {
        out << YAML::Key << "hover_click" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << config.hover_click->enabled;
        out << YAML::Key << "hover_duration" << YAML::Value << config.hover_click->hover_duration_milliseconds;
        out << YAML::Key << "cancel_displacement_threshold" << YAML::Value << config.hover_click->cancel_displacement_threshold;
        out << YAML::Key << "reclick_displacement_threshold" << YAML::Value << config.hover_click->reclick_displacement_threshold;
        out << YAML::EndMap;
    }

    if (!config.simulated_secondary_click.is_default_value)
    {
        out << YAML::Key << "simulated_secondary_click" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << config.simulated_secondary_click->enabled;
        out << YAML::Key << "hold_duration" << YAML::Value << config.simulated_secondary_click->hold_duration_milliseconds;
        out << YAML::Key << "displacement_threshold" << YAML::Value << config.simulated_secondary_click->displacement_threshold;
        out << YAML::EndMap;
    }

    if (!config.output_filter.is_default_value)
    {
        out << YAML::Key << "output_filter" << YAML::Value << YAML::BeginMap;
        if (config.output_filter->shader_path)
            out << YAML::Key << "shader_path" << YAML::Value << config.output_filter->shader_path.value();
        out << YAML::EndMap;
    }

    if (!config.cursor.is_default_value)
    {
        out << YAML::Key << "cursor" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "scale" << YAML::Value << config.cursor->scale;
        out << YAML::Key << "focus_mode" << YAML::Value << cursor_focus_mode_strings[static_cast<uint32_t>(config.cursor->focus_mode)];
        out << YAML::EndMap;
    }

    if (!config.slow_keys.is_default_value)
    {
        out << YAML::Key << "slow_keys" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << config.slow_keys->enabled;
        out << YAML::Key << "hold_delay" << YAML::Value << config.slow_keys->hold_delay_milliseconds;
        out << YAML::EndMap;
    }

    if (!config.sticky_keys.is_default_value)
    {
        out << YAML::Key << "sticky_keys" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << config.sticky_keys->enabled;
        out << YAML::Key << "should_disable_if_two_keys_are_pressed_together" << YAML::Value << config.sticky_keys->should_disable_if_two_keys_are_pressed_together;
        out << YAML::EndMap;
    }

    if (!config.magnifier.is_default_value)
    {
        out << YAML::Key << "magnifier" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "enabled" << YAML::Value << config.magnifier->enabled;
        out << YAML::Key << "scale" << YAML::Value << config.magnifier->scale;
        out << YAML::Key << "scale_increment" << YAML::Value << config.magnifier->scale_increment;
        out << YAML::Key << "width" << YAML::Value << config.magnifier->width;
        out << YAML::Key << "height" << YAML::Value << config.magnifier->height;
        out << YAML::Key << "size_increment" << YAML::Value << config.magnifier->size_increment;
        out << YAML::EndMap;
    }

    if (!config.workspace_back_and_forth.is_default_value)
    {
        out << YAML::Key << "workspace_back_and_forth" << YAML::Value << config.workspace_back_and_forth;
    }

    // Closing line
    out << YAML::EndMap;

    try
    {
        std::ofstream fout(path);
        fout.exceptions(std::ios::failbit | std::ios::badbit);
        if (fout.is_open())
            fout << out.c_str();
        else
            throw std::runtime_error("Error opening file");
    }
    catch (std::exception const& e)
    {
        result.success = false;
        result.errors.push_back({ -1, -1, ErrorLevel::error, path,
            std::string("Failed to save config: ") + e.what() });
    }

    return result;
}

std::string miracle::get_config_path()
{
    // $XDG_CONFIG_HOME/miracle-wm/config.yaml is where the configuration lives,
    // but $XDG_CONFIG_HOME/miracle-wm.yaml is the legacy file path. We first
    // check if the new path exists. If it does not, then we fall back to the old
    // path and only return it if it has contents written to it.
    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm/config.yaml";
    if (!std::filesystem::exists(config_path_stream.str()))
    {
        std::stringstream legacy_config_path_stream;
        legacy_config_path_stream << g_get_user_config_dir();
        legacy_config_path_stream << "/miracle-wm.yaml";
        if (std::filesystem::exists(legacy_config_path_stream.str()))
        {
            std::cerr << "Using the legacy file path: " << legacy_config_path_stream.str().c_str()
                      << ". Consider migrating to the new filepath at ~/.config/miracle-wm/config.yaml."
                      << std::endl;
            return legacy_config_path_stream.str();
        }
    }

    return config_path_stream.str();
}

std::string miracle::get_user_config_dir()
{
    return g_get_user_config_dir();
}

std::string miracle::get_display_config_path()
{
    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm/display.yaml";
    return config_path_stream.str();
}

std::string miracle::KeymapConfiguration::to_string() const
{
    std::stringstream ss;
    ss << language;
    if (variant)
    {
        ss << "+" << *variant;
    }
    else
    {
        // Warning: The Mir side of things expects some variant
        // if options are provided. If one isn't supplied, then
        // we supply the "empty" variant.
        ss << "+ ";
    }

    if (!options.empty())
    {
        ss << "+";
        for (size_t i = 0; i < options.size(); i++)
        {
            ss << options[i];
            if (i < options.size() - 1)
                ss << ",";
        }
    }

    return ss.str();
}

namespace
{
template <typename T>
std::vector<T> concat_vectors(std::vector<T> const& a, std::vector<T> const& b)
{
    std::vector<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

template <typename T, size_t U>
std::array<T, U> merge_arrays(
    std::array<T, U> const& a,
    std::array<T, U> const& b,
    std::function<bool(T const&, T const&)> const& compare)
{
    std::array<T, U> result;
    for (size_t i = 0; i < a.size(); i++)
    {
        auto const& a_item = a[i];
        auto const& b_item = b[i];
        result[i] = compare(a_item, b_item) ? a_item : b_item;
    }

    return result;
}
}

miracle::ConfigData miracle::ConfigData::merge_with(miracle::ConfigData& other)
{
    // This method merges two configurations. [other] will be given priority over [this]
    // if it is set.
    ConfigData result;
    result.primary_modifier = other.primary_modifier.is_set() ? other.primary_modifier : primary_modifier;
    result.primary_button = other.primary_button.is_set() ? other.primary_button : primary_button;
    result.custom_key_commands = concat_vectors(*other.custom_key_commands, *custom_key_commands);
    result.built_in_key_command_overrides = concat_vectors(*other.built_in_key_command_overrides, *built_in_key_command_overrides);
    result.inner_gaps = other.inner_gaps.is_set() ? other.inner_gaps : inner_gaps;
    result.outer_gaps = other.outer_gaps.is_set() ? other.outer_gaps : outer_gaps;
    result.startup_apps = concat_vectors(*other.startup_apps, *startup_apps);
    result.terminal = other.terminal.is_set() ? other.terminal : terminal;
    result.resize_jump = other.resize_jump.is_set() ? other.resize_jump : resize_jump;
    result.environment_variables = concat_vectors(*other.environment_variables, *environment_variables);
    result.border_config = other.border_config.is_set() ? other.border_config : border_config;
    result.animations_enabled = other.animations_enabled.is_set() ? other.animations_enabled : animations_enabled;
    result.animation_definitions = other.animation_definitions.is_set() ? other.animation_definitions : animation_definitions;
    result.workspace_configs = concat_vectors(*other.workspace_configs, *workspace_configs);
    result.move_modifier = other.move_modifier.is_set() ? other.move_modifier : move_modifier;
    result.drag_and_drop = other.drag_and_drop.is_set() ? other.drag_and_drop : drag_and_drop;
    result.mouse_configuration->merge(*other.mouse_configuration);
    result.mouse_configuration->merge(*mouse_configuration);
    result.keyboard_configuration->merge(*other.keyboard_configuration);
    result.keyboard_configuration->merge(*keyboard_configuration);
    result.touchpad = other.touchpad.is_set() ? other.touchpad : touchpad;
    result.keymap = other.keymap.is_set() ? other.keymap : keymap;
    result.hover_click = other.hover_click.is_set() ? other.hover_click : hover_click;
    result.simulated_secondary_click = other.simulated_secondary_click.is_set() ? other.simulated_secondary_click : simulated_secondary_click;
    result.output_filter = other.output_filter.is_set() ? other.output_filter : output_filter;
    result.cursor = other.cursor.is_set() ? other.cursor : cursor;
    result.slow_keys = other.slow_keys.is_set() ? other.slow_keys : slow_keys;
    result.sticky_keys = other.sticky_keys.is_set() ? other.sticky_keys : sticky_keys;
    result.includes = concat_vectors(*other.includes, *includes);
    result.magnifier = other.magnifier.is_set() ? other.magnifier : magnifier;
    result.workspace_back_and_forth = other.workspace_back_and_forth.is_set() ? other.workspace_back_and_forth : workspace_back_and_forth;
    result.plugins = other.plugins.is_set() ? other.plugins : plugins;
    return result;
}

miracle::AnimationDefinition miracle::ConfigData::get_default_animation_definition(AnimateableEvent event)
{
    return default_animation_definitions[static_cast<uint32_t>(event)];
}
