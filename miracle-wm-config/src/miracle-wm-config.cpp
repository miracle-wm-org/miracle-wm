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

#include "miracle/miracle-wm-config.h"
#include "miracle/animation_definition_internal.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <glib-2.0/glib.h>
#include <glm/fwd.hpp>
#include <iostream>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

namespace YAML
{
class BadConversion;
}

namespace
{
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

std::optional<miracle::ContainerType> container_type_from_string(std::string const& str, ParsingContext& context)
{
    for (auto i = 0; i < miracle::container_type_strings.size(); i++)
    {
        if (miracle::container_type_strings[i] == str)
            return static_cast<miracle::ContainerType>(i);
    }

    return std::nullopt;
}

std::optional<miracle::AnimateableEvent> from_string_animateable_event(std::string const& str, ParsingContext& context)
{
    for (auto i = 0; i < miracle::animateable_event_strings.size(); i++)
    {
        if (miracle::animateable_event_strings[i] == str)
            return static_cast<miracle::AnimateableEvent>(i);
    }

    return std::nullopt;
}

std::optional<miracle::EaseFunction> from_string_ease_function(std::string const& str, ParsingContext& context)
{
    for (auto i = 0; i < miracle::ease_function_strings.size(); i++)
    {
        if (miracle::ease_function_strings[i] == str)
            return static_cast<miracle::EaseFunction>(i);
    }

    return std::nullopt;
}

std::optional<miracle::AnimationType> from_string_animation_type(std::string const& str, ParsingContext& context)
{
    for (auto i = 0; i < miracle::animation_type_strings.size(); i++)
    {
        if (miracle::animation_type_strings[i] == str)
            return static_cast<miracle::AnimationType>(i);
    }

    return std::nullopt;
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
            unsigned int const i = std::stoul(value, nullptr, 16);
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
        for (auto i = 0; i < miracle::default_key_command_strings.size(); i++)
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

        context.result.config.built_in_key_command_overrides.push_back({ keyboard_action.value(),
            modifiers,
            code,
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

        context.result.config.custom_key_commands.push_back({ keyboard_action.value(),
            modifiers,
            code,
            command });
    }
}

void read_inner_gaps(YAML::Node const& node, ParsingContext& context)
{
    if (!try_parse_value(node, "x", context.result.config.inner_gaps_x, context))
        return;
    if (!try_parse_value(node, "y", context.result.config.inner_gaps_y, context))
        return;
}

void read_outer_gaps(YAML::Node const& node, ParsingContext& context)
{
    if (!try_parse_value(node, "x", context.result.config.outer_gaps_x, context))
        return;
    if (!try_parse_value(node, "y", context.result.config.outer_gaps_y, context))
        return;
}

void read_startup_apps(YAML::Node const& startup_apps, ParsingContext& context)
{
    if (!startup_apps.IsSequence())
    {
        context.builder << "Expected startup applications to be an array";
        create_error(startup_apps, context);
        return;
    }

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

        context.result.config.startup_apps.push_back({ .command = std::move(command),
            .restart_on_death = restart_on_death,
            .no_startup_id = no_startup_id,
            .should_halt_compositor_on_death = should_halt_compositor_on_death,
            .in_systemd_scope = in_systemd_scope });
    }
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
    try_parse_value(node, context.result.config.resize_jump, context);
}

void read_environment_variables(YAML::Node const& env, ParsingContext& context)
{
    if (!env.IsSequence())
    {
        context.builder << "Expected environment variables to be an array";
        create_error(env, context);
        return;
    }

    for (auto const& node : env)
    {
        std::string key, value;
        if (!try_parse_value(node, "key", key, context))
            continue;
        if (!try_parse_value(node, "value", value, context))
            continue;
        context.result.config.environment_variables.push_back({ key, value });
    }
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

    for (auto const& workspace : workspaces)
    {
        int num;
        if (!try_parse_value(workspace, "number", num, context))
            continue;

        std::optional<miracle::ContainerType> type;
        if (workspace["key"])
        {
            type = try_parse_string_to_optional_value<std::optional<miracle::ContainerType>>(workspace, "layout", container_type_from_string, context);
            if (!type || type.value() == miracle::ContainerType::none)
                continue;
        }

        std::string name;
        if (!try_parse_value(workspace, "name", name, context, true))
            continue;

        context.result.config.workspace_configs.push_back({ num,
            type,
            name.empty() ? std::optional<std::string>(std::nullopt) : name });
    }
}

void read_animation_definitions(YAML::Node const& animations_node, ParsingContext& context)
{
    if (!animations_node.IsSequence())
    {
        context.builder << "Animation definitions must be a sequence";
        create_error(animations_node, context);
        return;
    }

    for (auto const& node : animations_node)
    {
        auto const& event = try_parse_string_to_optional_value<std::optional<miracle::AnimateableEvent>>(
            node,
            "event",
            from_string_animateable_event,
            context);
        if (!event)
            continue;

        auto const& type = try_parse_string_to_optional_value<std::optional<miracle::AnimationType>>(
            node,
            "type",
            from_string_animation_type,
            context);
        if (!type)
            continue;

        auto const& function = try_parse_string_to_optional_value<std::optional<miracle::EaseFunction>>(
            node,
            "function",
            from_string_ease_function,
            context);
        if (!function)
            continue;

        int const event_as_int = static_cast<int>(event.value());
        context.result.config.animation_definitions[event_as_int].is_default = false;
        context.result.config.animation_definitions[event_as_int].type = type.value();
        context.result.config.animation_definitions[event_as_int].function = function.value();
        try_parse_value(node, "duration", context.result.config.animation_definitions[event_as_int].duration_seconds, context, true);
        try_parse_value(node, "c1", context.result.config.animation_definitions[event_as_int].c1, context, true);
        try_parse_value(node, "c2", context.result.config.animation_definitions[event_as_int].c2, context, true);
        try_parse_value(node, "c3", context.result.config.animation_definitions[event_as_int].c3, context, true);
        try_parse_value(node, "c4", context.result.config.animation_definitions[event_as_int].c4, context, true);
        try_parse_value(node, "n1", context.result.config.animation_definitions[event_as_int].n1, context, true);
        try_parse_value(node, "d1", context.result.config.animation_definitions[event_as_int].d1, context, true);
    }
}

void read_enable_animations(YAML::Node const& node, ParsingContext& context)
{
    try_parse_value(node, context.result.config.animations_enabled, context);
}

void read_move_modifier(YAML::Node const& node, ParsingContext& context)
{
    try_parse_modifiers(node, context.result.config.move_modifier, context);
}

void read_drag_and_drop(YAML::Node const& node, ParsingContext& context)
{
    try_parse_value(node, "enabled", context.result.config.drag_and_drop.enabled, context, true);
    uint modifiers = 0;
    if (node["modifiers"])
    {
        if (!try_parse_modifiers(node["modifiers"], modifiers, context))
            return;

        context.result.config.drag_and_drop.modifiers = modifiers;
    }
}
}

miracle::ConfigData::ConfigData()
{
    animation_definitions = internal::default_animation_definitions;
}

miracle::ConfigLoadResult miracle::load_config(std::string const& path)
{
    ParsingContext context;

    try
    {
        YAML::Node config = YAML::LoadFile(path);
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
    ConfigSaveResult result(true);
    YAML::Emitter out;
    out << YAML::BeginMap;

    // Save primary modifier
    for (auto const& [name, value] : mir_input_event_modifier_opts)
    {
        if (value == config.primary_modifier)
        {
            out << YAML::Key << "action_key" << YAML::Value << name;
            break;
        }
    }

    // Save default action overrides
    if (!config.built_in_key_command_overrides.empty())
    {
        out << YAML::Key << "default_action_overrides" << YAML::Value << YAML::BeginSeq;
        for (auto const& override : config.built_in_key_command_overrides)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << default_key_command_strings[static_cast<int>(override.default_key_command)];
            out << YAML::Key << "action" << YAML::Value << mir_keyboard_actions_strings[override.action].first;
            out << YAML::Key << "key" << YAML::Value << libevdev_event_code_get_name(EV_KEY, override.key);

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
    if (!config.custom_key_commands.empty())
    {
        out << YAML::Key << "custom_actions" << YAML::Value << YAML::BeginSeq;
        for (auto const& action : config.custom_key_commands)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "command" << YAML::Value << action.command;
            out << YAML::Key << "action" << YAML::Value << mir_keyboard_actions_strings[action.action].first;
            out << YAML::Key << "key" << YAML::Value << libevdev_event_code_get_name(EV_KEY, action.key);

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
    out << YAML::Key << "inner_gaps" << YAML::Value << YAML::BeginMap
        << YAML::Key << "x" << YAML::Value << config.inner_gaps_x
        << YAML::Key << "y" << YAML::Value << config.inner_gaps_y
        << YAML::EndMap;

    out << YAML::Key << "outer_gaps" << YAML::Value << YAML::BeginMap
        << YAML::Key << "x" << YAML::Value << config.outer_gaps_x
        << YAML::Key << "y" << YAML::Value << config.outer_gaps_y
        << YAML::EndMap;

    // Save startup apps
    if (!config.startup_apps.empty())
    {
        out << YAML::Key << "startup_apps" << YAML::Value << YAML::BeginSeq;
        for (auto const& app : config.startup_apps)
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
    if (config.terminal)
        out << YAML::Key << "terminal" << YAML::Value << config.terminal.value();

    // Save resize jump
    out << YAML::Key << "resize_jump" << YAML::Value << config.resize_jump;

    // Save environment variables
    if (!config.environment_variables.empty())
    {
        out << YAML::Key << "environment_variables" << YAML::Value << YAML::BeginSeq;
        for (auto const& var : config.environment_variables)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "key" << YAML::Value << var.key;
            out << YAML::Key << "value" << YAML::Value << var.value;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Save border config
    if (config.border_config.size > 0)
    {
        out << YAML::Key << "border" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "size" << YAML::Value << config.border_config.size;
        out << YAML::Key << "radius" << YAML::Value << config.border_config.radius;

        // Save colors as hex values
        auto to_hex = [](glm::vec4 const& color)
        {
            return ((int)(color.r * 255) << 24) | ((int)(color.g * 255) << 16) | ((int)(color.b * 255) << 8) | ((int)(color.a * 255));
        };

        out << YAML::Key << "color" << YAML::Value << YAML::Hex << to_hex(config.border_config.color);
        out << YAML::Key << "focus_color" << YAML::Value << YAML::Hex << to_hex(config.border_config.focus_color);
        out << YAML::EndMap;
    }

    // Save workspaces
    if (!config.workspace_configs.empty())
    {
        out << YAML::Key << "workspaces" << YAML::Value << YAML::BeginSeq;
        for (auto const& workspace : config.workspace_configs)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "number" << YAML::Value << workspace.num.value();
            if (workspace.layout)
                out << YAML::Key << "layout" << YAML::Value << container_type_strings[static_cast<int>(workspace.layout.value())];
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
    for (auto const& def : config.animation_definitions)
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
        for (int i = 0; i < static_cast<int>(AnimateableEvent::max); i++)
        {
            auto const& def = config.animation_definitions[i];
            if (!def.is_default)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "event" << YAML::Value << animateable_event_strings[i];
                out << YAML::Key << "type" << YAML::Value << animation_type_strings[static_cast<int>(def.type)];
                out << YAML::Key << "function" << YAML::Value << ease_function_strings[static_cast<int>(def.function)];
                if (def.duration_seconds != 0.f)
                    out << YAML::Key << "duration" << YAML::Value << def.duration_seconds;
                if (def.c1 != 0.f)
                    out << YAML::Key << "c1" << YAML::Value << def.c1;
                if (def.c2 != 0.f)
                    out << YAML::Key << "c2" << YAML::Value << def.c2;
                if (def.c3 != 0.f)
                    out << YAML::Key << "c3" << YAML::Value << def.c3;
                if (def.c4 != 0.f)
                    out << YAML::Key << "c4" << YAML::Value << def.c4;
                if (def.n1 != 0.f)
                    out << YAML::Key << "n1" << YAML::Value << def.n1;
                if (def.d1 != 0.f)
                    out << YAML::Key << "d1" << YAML::Value << def.d1;
                out << YAML::EndMap;
            }
        }
        out << YAML::EndSeq;
    }

    // Save move modifier
    if (config.move_modifier != miracle_input_event_modifier_default)
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
    if (!config.drag_and_drop.enabled || config.drag_and_drop.modifiers != (miracle_input_event_modifier_default | mir_input_event_modifier_shift))
    {
        out << YAML::Key << "drag_and_drop" << YAML::Value << YAML::BeginMap;
        if (!config.drag_and_drop.enabled)
            out << YAML::Key << "enabled" << YAML::Value << config.drag_and_drop.enabled;

        if (config.drag_and_drop.modifiers != (miracle_input_event_modifier_default | mir_input_event_modifier_shift))
        {
            out << YAML::Key << "modifiers" << YAML::Value << YAML::BeginSeq;
            for (auto const& [name, value] : mir_input_event_modifier_opts)
            {
                if (config.drag_and_drop.modifiers & value)
                    out << name;
            }
            out << YAML::EndSeq;
        }
        out << YAML::EndMap;
    }

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

    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm/config.yaml";
    return config_path_stream.str();
}

std::string miracle::get_display_config_path()
{
    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm/display.yaml";
    return config_path_stream.str();
}
