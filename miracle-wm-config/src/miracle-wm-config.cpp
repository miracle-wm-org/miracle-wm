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
#include <cstdlib>
#include <fstream>
#include <functional>
#include <glm/fwd.hpp>
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
    for (auto i = 0; i < miracle::mir_keyboard_actions_strings.size(); i++)
    {
        if (miracle::mir_keyboard_actions_strings[i] == str)
            return static_cast<MirKeyboardAction>(i);
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
    if (str == "primary")
        return miracle::miracle_input_event_modifier_default;

    for (auto i = 0; i < miracle::mir_input_event_modifier_strings.size(); i++)
    {
        if (miracle::mir_input_event_modifier_strings[i] == str)
            return 1 << i;
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

        context.result.config.key_commands[static_cast<int>(key_command)].push_back({ keyboard_action.value(),
            modifiers,
            code });
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

        context.result.config.startup_apps.push_back({ .command = std::move(command),
            .restart_on_death = restart_on_death,
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
    int size;
    if (!try_parse_value(border, "size", size, context))
        return;

    glm::vec4 color;
    if (!try_parse_color(border, "color", color, context))
        return;

    glm::vec4 focus_color;
    if (!try_parse_color(border, "focus_color", focus_color, context))
        return;

    context.result.config.border_config = { size, focus_color, color };
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
    const KeyCommand default_key_commands[static_cast<int>(DefaultKeyCommand::MAX)] = {
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_ENTER },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_V     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_H     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_R     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_UP    },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_DOWN  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_LEFT  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_RIGHT },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_UP    },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_DOWN  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_LEFT  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_RIGHT },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_UP    },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_DOWN  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_LEFT  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_RIGHT },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_Q     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_E     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_F     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_1     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_2     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_3     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_4     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_5     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_6     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_7     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_8     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_9     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_0     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_1     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_2     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_3     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_4     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_5     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_6     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_7     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_8     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_9     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_0     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_SPACE },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_P     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_W     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_S     }
    };
    for (int i = 0; i < static_cast<int>(DefaultKeyCommand::MAX); i++)
    {
        if (key_commands[i].empty())
            key_commands[i].push_back(default_key_commands[i]);
    }

    std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> parsed({
        {
         AnimationType::fade_in,
         EaseFunction::linear,
         0.3f,
         },
        {
         AnimationType::slide,
         EaseFunction::linear,
         0.25f,
         },
        {
         AnimationType::fade_out,
         EaseFunction::linear,
         0.3f,
         },
        { AnimationType::slide,
         EaseFunction::ease_out_sine,
         0.25f }
    });
    animation_definitions = parsed;
}

miracle::ConfigLoadResult miracle::load_config(std::string const& path)
{
    ParsingContext context;

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

    return context.result;
}
