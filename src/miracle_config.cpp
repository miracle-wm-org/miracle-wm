#define MIR_LOG_COMPONENT "miracle_config"

#include "miracle_config.h"
#include "yaml-cpp/yaml.h"
#include <glib-2.0/glib.h>
#include <fstream>
#include <mir/log.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libnotify/notify.h>

using namespace miracle;

namespace
{
int program_exists(std::string const& name)
{
    std::stringstream out;
    out << "command -v " << name << " > /dev/null 2>&1";
    return !system(out.str().c_str());
}
}

MiracleConfig::MiracleConfig()
{
    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm.yaml";
    auto config_path = config_path_stream.str();

    mir::log_info("Configuration file path is: %s", config_path.c_str());

    {
    std::fstream file(config_path, std::ios::out | std::ios::in | std::ios::app);
    }

    YAML::Node config = YAML::LoadFile(config_path);
    if (config["action_key"])
    {
        auto const stringified_action_key = config["action_key"].as<std::string>();
        auto modifier = parse_modifier(stringified_action_key);
        if (modifier == mir_input_event_modifier_none)
            mir::log_error("action_key: invalid action key: %s", stringified_action_key.c_str());
        else
            primary_modifier = parse_modifier(stringified_action_key);
    }

    if (config["default_action_overrides"])
    {
        auto const default_action_overrides = config["default_action_overrides"];
        if (!default_action_overrides.IsSequence())
        {
            mir::log_error("default_action_overrides: value must be an array");
            return;
        }

        for (auto i = 0; i < default_action_overrides.size(); i++)
        {
            auto sub_node = default_action_overrides[i];
            if (!sub_node["name"])
            {
                mir::log_error("default_action_overrides: missing name");
                continue;
            }

            if (!sub_node["action"])
            {
                mir::log_error("default_action_overrides: missing action");
                continue;
            }

            if (!sub_node["modifiers"])
            {
                mir::log_error("default_action_overrides: missing modifiers");
                continue;
            }

            if (!sub_node["key"])
            {
                mir::log_error("default_action_overrides: missing key");
                continue;
            }

            auto name = sub_node["name"].as<std::string>();
            DefaultKeyCommand key_command;
            if (name == "terminal")
                key_command = DefaultKeyCommand::Terminal;
            else if (name == "request_vertical")
                key_command = DefaultKeyCommand::RequestVertical;
            else if (name == "request_horizontal")
                key_command = DefaultKeyCommand::RequestHorizontal;
            else if (name == "toggle_resize")
                key_command = DefaultKeyCommand::ToggleResize;
            else if (name == "move_up")
                key_command = DefaultKeyCommand::MoveUp;
            else if (name == "move_down")
                key_command = DefaultKeyCommand::MoveDown;
            else if (name == "move_left")
                key_command = DefaultKeyCommand::MoveLeft;
            else if (name == "move_right")
                key_command = DefaultKeyCommand::MoveRight;
            else if (name == "select_up")
                key_command = DefaultKeyCommand::SelectUp;
            else if (name == "select_down")
                key_command = DefaultKeyCommand::SelectDown;
            else if (name == "select_left")
                key_command = DefaultKeyCommand::SelectLeft;
            else if (name == "select_right")
                key_command = DefaultKeyCommand::SelectRight;
            else if (name == "quit_active_window")
                key_command = DefaultKeyCommand::QuitActiveWindow;
            else if (name == "quit_compositor")
                key_command = DefaultKeyCommand::QuitCompositor;
            else if (name == "fullscreen")
                key_command = DefaultKeyCommand::Fullscreen;
            else if (name == "select_workspace_1")
                key_command = DefaultKeyCommand::SelectWorkspace1;
            else if (name == "select_workspace_2")
                key_command = DefaultKeyCommand::SelectWorkspace2;
            else if (name == "select_workspace_3")
                key_command = DefaultKeyCommand::SelectWorkspace3;
            else if (name == "select_workspace_4")
                key_command = DefaultKeyCommand::SelectWorkspace4;
            else if (name == "select_workspace_5")
                key_command = DefaultKeyCommand::SelectWorkspace5;
            else if (name == "select_workspace_6")
                key_command = DefaultKeyCommand::SelectWorkspace6;
            else if (name == "select_workspace_7")
                key_command = DefaultKeyCommand::SelectWorkspace7;
            else if (name == "select_workspace_8")
                key_command = DefaultKeyCommand::SelectWorkspace8;
            else if (name == "select_workspace_9")
                key_command = DefaultKeyCommand::SelectWorkspace9;
            else if (name == "select_workspace_0")
                key_command = DefaultKeyCommand::SelectWorkspace0;
            else if (name == "move_to_workspace_1")
                key_command = DefaultKeyCommand::MoveToWorkspace1;
            else if (name == "move_to_workspace_2")
                key_command = DefaultKeyCommand::MoveToWorkspace2;
            else if (name == "move_to_workspace_3")
                key_command = DefaultKeyCommand::MoveToWorkspace3;
            else if (name == "move_to_workspace_4")
                key_command = DefaultKeyCommand::MoveToWorkspace4;
            else if (name == "move_to_workspace_5")
                key_command = DefaultKeyCommand::MoveToWorkspace5;
            else if (name == "move_to_workspace_6")
                key_command = DefaultKeyCommand::MoveToWorkspace6;
            else if (name == "move_to_workspace_7")
                key_command = DefaultKeyCommand::MoveToWorkspace7;
            else if (name == "move_to_workspace_8")
                key_command = DefaultKeyCommand::MoveToWorkspace8;
            else if (name == "move_to_workspace_9")
                key_command = DefaultKeyCommand::MoveToWorkspace9;
            else if (name == "move_to_workspace_0")
                key_command = DefaultKeyCommand::MoveToWorkspace0;
            else {
                mir::log_error("default_action_overrides: Unknown key command override: %s", name.c_str());
                continue;
            }

            auto action = sub_node["action"].as<std::string>();
            MirKeyboardAction keyboard_action;
            if (action == "up")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_up;
            else if (action == "down")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_down;
            else if (action == "repeat")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_repeat;
            else if (action == "modifiers")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_modifiers;
            else {
                mir::log_error("default_action_overrides: Unknown keyboard action: %s", action.c_str());
                continue;
            }

            auto key = sub_node["key"].as<std::string>();
            auto code = libevdev_event_code_from_name(EV_KEY, key.c_str()); //https://stackoverflow.com/questions/32059363/is-there-a-way-to-get-the-evdev-keycode-from-a-string
            if (code < 0)
            {
                mir::log_error("default_action_overrides: Unknown keyboard code in configuration: %s. See the linux kernel for allowed codes: https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h", key.c_str());
                continue;
            }

            auto modifiers_node = sub_node["modifiers"];

            if (!modifiers_node.IsSequence())
            {
                mir::log_error("default_action_overrides: Provided modifiers is not an array");
                continue;
            }

            uint modifiers = 0;
            for (auto j = 0; j < modifiers_node.size(); j++)
            {
                auto modifier = modifiers_node[j].as<std::string>();
                modifiers = modifiers | parse_modifier(modifier);
            }

            key_commands[key_command].push_back({
                keyboard_action,
                modifiers,
                code
            });
        }
    }

    // Fill in default actions if the list is zero for any one of them
    const KeyCommand default_key_commands[DefaultKeyCommand::MAX] =
    {
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_ENTER
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_V
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_H
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_R
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_UP
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_DOWN
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_LEFT
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_RIGHT
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_UP
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_DOWN
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_LEFT
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_RIGHT
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_Q
        },
        {
            MirKeyboardAction::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_E
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_F
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_1
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_2
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_3
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_4
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_5
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_6
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_7
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_8
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_9
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default,
            KEY_0
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_1
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_2
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_3
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_4
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_5
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_6
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_7
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_8
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_9
        },
        {
            MirKeyboardAction ::mir_keyboard_action_down,
            miracle_input_event_modifier_default | mir_input_event_modifier_shift,
            KEY_0
        }
    };
    for (int i = 0; i < DefaultKeyCommand::MAX; i++)
    {
        if (key_commands[i].empty())
            key_commands[i].push_back(default_key_commands[i]);
    }

    // Custom actions
    if (config["custom_actions"])
    {
        auto const custom_actions = config["custom_actions"];
        if (!custom_actions.IsSequence())
        {
            mir::log_error("custom_actions: value must be an array");
            return;
        }

        for (auto i = 0; i < custom_actions.size(); i++)
        {
            auto sub_node = custom_actions[i];
            if (!sub_node["command"])
            {
                mir::log_error("custom_actions: missing command");
                continue;
            }

            if (!sub_node["action"])
            {
                mir::log_error("custom_actions: missing action");
                continue;
            }

            if (!sub_node["modifiers"])
            {
                mir::log_error("custom_actions: missing modifiers");
                continue;
            }

            if (!sub_node["key"])
            {
                mir::log_error("custom_actions: missing key");
                continue;
            }

            // TODO: Copy & paste here
            auto command = sub_node["command"].as<std::string>();
            auto action = sub_node["action"].as<std::string>();
            MirKeyboardAction keyboard_action;
            if (action == "up")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_up;
            else if (action == "down")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_down;
            else if (action == "repeat")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_repeat;
            else if (action == "modifiers")
                keyboard_action = MirKeyboardAction::mir_keyboard_action_modifiers;
            else {
                mir::log_error("custom_actions: Unknown keyboard action: %s", action.c_str());
                continue;
            }

            auto key = sub_node["key"].as<std::string>();
            auto code = libevdev_event_code_from_name(EV_KEY,
                                                      key.c_str()); //https://stackoverflow.com/questions/32059363/is-there-a-way-to-get-the-evdev-keycode-from-a-string
            if (code < 0)
            {
                mir::log_error(
                    "custom_actions: Unknown keyboard code in configuration: %s. See the linux kernel for allowed codes: https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h",
                    key.c_str());
                continue;
            }

            auto modifiers_node = sub_node["modifiers"];

            if (!modifiers_node.IsSequence())
            {
                mir::log_error("custom_actions: Provided modifiers is not an array");
                continue;
            }

            uint modifiers = 0;
            for (auto j = 0; j < modifiers_node.size(); j++)
            {
                auto modifier = modifiers_node[j].as<std::string>();
                modifiers = modifiers | parse_modifier(modifier);
            }

            custom_key_commands.push_back({
                keyboard_action,
                modifiers,
                code,
                command
            });
        }
    }

    // Gap sizes
    if (config["gap_size_x"])
    {
        gap_size_x = config["gap_size_x"].as<int>();
    }
    if (config["gap_size_y"])
    {
        gap_size_y = config["gap_size_y"].as<int>();
    }

    // Startup Apps
    if (config["startup_apps"])
    {
        for (auto const& node : config["startup_apps"])
        {
            if (!node["command"])
            {
                mir::log_error("startup_apps: app lacks a command");
                continue;
            }

            auto command = node["command"].as<std::string>();
            bool restart_on_death = false;
            if (node["restart_on_death"])
            {
                restart_on_death = node["restart_on_death"].as<bool>();
            }

            startup_apps.push_back({std::move(command), restart_on_death});
        }
    }

    if (config["terminal"])
    {
        terminal = config["terminal"].as<std::string>();
    }

    if (terminal && !program_exists(terminal.value()))
    {
        desired_terminal = terminal.value();
        terminal.reset();
    }
}

uint MiracleConfig::parse_modifier(std::string const& stringified_action_key)
{
    if (stringified_action_key == "alt")
        return mir_input_event_modifier_alt;
    else if (stringified_action_key == "alt_left")
        return mir_input_event_modifier_alt_left;
    else if (stringified_action_key == "alt_right")
        return mir_input_event_modifier_alt_right;
    else if (stringified_action_key == "shift")
        return mir_input_event_modifier_shift;
    else if (stringified_action_key == "shift_left")
        return mir_input_event_modifier_shift_left;
    else if (stringified_action_key == "shift_right")
        return mir_input_event_modifier_shift_right;
    else if (stringified_action_key == "sym")
        return mir_input_event_modifier_sym;
    else if (stringified_action_key == "function")
        return mir_input_event_modifier_function;
    else if (stringified_action_key == "ctrl")
        return mir_input_event_modifier_ctrl;
    else if (stringified_action_key == "ctrl_left")
        return mir_input_event_modifier_ctrl_left;
    else if (stringified_action_key == "ctrl_right")
        return mir_input_event_modifier_ctrl_right;
    else if (stringified_action_key == "meta")
        return mir_input_event_modifier_meta;
    else if (stringified_action_key == "meta_left")
        return mir_input_event_modifier_meta_left;
    else if (stringified_action_key == "meta_right")
        return mir_input_event_modifier_meta_right;
    else if (stringified_action_key == "caps_lock")
        return mir_input_event_modifier_caps_lock;
    else if (stringified_action_key == "num_lock")
        return mir_input_event_modifier_num_lock;
    else if (stringified_action_key == "scroll_lock")
        return mir_input_event_modifier_scroll_lock;
    else if (stringified_action_key == "primary")
        return miracle_input_event_modifier_default;
    else
        mir::log_error("Unable to process action_key: %s", stringified_action_key.c_str());
    return mir_input_event_modifier_none;
}

MirInputEventModifier MiracleConfig::get_input_event_modifier() const
{
    return (MirInputEventModifier)primary_modifier;
}

CustomKeyCommand const*
MiracleConfig::matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const
{
    // TODO: Copy & paste
    for (auto const& command : custom_key_commands)
    {
        if (action != command.action)
            continue;

        auto command_modifiers = command.modifiers;
        if (command_modifiers & miracle_input_event_modifier_default)
            command_modifiers = command_modifiers & ~miracle_input_event_modifier_default | get_input_event_modifier();

        if (command_modifiers != modifiers)
            continue;

        if (scan_code == command.key)
            return &command;
    }

    return nullptr;
}

DefaultKeyCommand MiracleConfig::matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const
{
    for (int i = 0; i < DefaultKeyCommand::MAX; i++)
    {
        for (auto command : key_commands[i])
        {
            if (action != command.action)
                continue;

            auto command_modifiers = command.modifiers;
            if (command_modifiers & miracle_input_event_modifier_default)
                command_modifiers = command_modifiers & ~miracle_input_event_modifier_default | get_input_event_modifier();

            if (command_modifiers != modifiers)
                continue;

            if (scan_code == command.key)
                return (DefaultKeyCommand)i;
        }
    }

    return DefaultKeyCommand::MAX;
}

int MiracleConfig::get_gap_size_x() const
{
    return gap_size_x;
}

int MiracleConfig::get_gap_size_y() const
{
    return gap_size_y;
}

const std::vector<StartupApp> &MiracleConfig::get_startup_apps() const
{
    return startup_apps;
}

const std::optional<std::string> &MiracleConfig::get_terminal_command() const
{
    if (!terminal)
    {
        auto error_string = "Terminal program does not exist " + desired_terminal;
        mir::log_error("%s", error_string.c_str());
        NotifyNotification* n = notify_notification_new(
            "Terminal program does not exist",
             error_string.c_str(),
            nullptr);
        notify_notification_set_timeout(n, 5000);
        notify_notification_show(n, nullptr);
    }
    return terminal;
}