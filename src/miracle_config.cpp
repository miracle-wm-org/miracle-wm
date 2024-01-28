#define MIR_LOG_COMPONENT "miracle_config"

#include "miracle_config.h"
#include "yaml-cpp/yaml.h"
#include <glib-2.0/glib.h>
#include <fstream>
#include <mir/log.h>

using namespace miracle;

MiracleConfig::MiracleConfig()
{
    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm.yaml";
    auto config_path = config_path_stream.str();

    {
    std::fstream file(config_path, std::ios::out | std::ios::in | std::ios::app);
    }
    YAML::Node config = YAML::LoadFile(config_path);
    if (config["action_key"])
    {
        auto const stringified_action_key = config["action_key"].as<std::string>();
        if (stringified_action_key == "alt")
            primary_modifier = mir_input_event_modifier_alt;
        else if (stringified_action_key == "alt_left")
            primary_modifier = mir_input_event_modifier_alt_left;
        else if (stringified_action_key == "alt_right")
            primary_modifier = mir_input_event_modifier_alt_right;
        else if (stringified_action_key == "shift")
            primary_modifier = mir_input_event_modifier_shift;
        else if (stringified_action_key == "shift_left")
            primary_modifier = mir_input_event_modifier_shift_left;
        else if (stringified_action_key == "shift_right")
            primary_modifier = mir_input_event_modifier_shift_right;
        else if (stringified_action_key == "sym")
            primary_modifier = mir_input_event_modifier_sym;
        else if (stringified_action_key == "function")
            primary_modifier = mir_input_event_modifier_function;
        else if (stringified_action_key == "ctrl")
            primary_modifier = mir_input_event_modifier_ctrl;
        else if (stringified_action_key == "ctrl_left")
            primary_modifier = mir_input_event_modifier_ctrl_left;
        else if (stringified_action_key == "ctrl_right")
            primary_modifier = mir_input_event_modifier_ctrl_right;
        else if (stringified_action_key == "meta")
            primary_modifier = mir_input_event_modifier_meta;
        else if (stringified_action_key == "meta_left")
            primary_modifier = mir_input_event_modifier_meta_left;
        else if (stringified_action_key == "meta_right")
            primary_modifier = mir_input_event_modifier_meta_right;
        else if (stringified_action_key == "caps_lock")
            primary_modifier = mir_input_event_modifier_caps_lock;
        else if (stringified_action_key == "num_lock")
            primary_modifier = mir_input_event_modifier_num_lock;
        else if (stringified_action_key == "scroll_lock")
            primary_modifier = mir_input_event_modifier_num_lock;
        else
            mir::log_error("Unable to process action_key: %s", stringified_action_key.c_str());
    }
}

MirInputEventModifier MiracleConfig::get_input_event_modifier() const
{
    return primary_modifier;
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