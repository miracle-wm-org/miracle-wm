#include "miracle_config.h"

using namespace miracle;

MiracleConfig::MiracleConfig()
{

}

MirInputEventModifier MiracleConfig::get_input_event_modifier() const
{
    return modifier;
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