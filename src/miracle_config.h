#ifndef MIRACLEWM_MIRACLE_CONFIG_H
#define MIRACLEWM_MIRACLE_CONFIG_H

#include <string>
#include <miral/toolkit_event.h>
#include <vector>
#include <linux/input.h>

namespace miracle
{

enum DefaultKeyCommand
{
    Terminal = 0,
    RequestVertical,
    RequestHorizontal,
    ToggleResize,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    SelectUp,
    SelectDown,
    SelectLeft,
    SelectRight,
    QuitActiveWindow,
    QuitCompositor,
    MAX
};


struct KeyCommand
{
    MirKeyboardAction action;
    uint modifiers;
    int key;
};

typedef std::vector<KeyCommand> KeyCommandList;

class MiracleConfig
{
public:
    MiracleConfig();
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const;
    [[nodiscard]] DefaultKeyCommand matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const;

private:
    static uint parse_modifier(std::string const& stringified_action_key);

    static const uint miracle_input_event_modifier_default = 1 << 18;
    uint primary_modifier = mir_input_event_modifier_meta;
    KeyCommandList key_commands[DefaultKeyCommand::MAX];
};
}


#endif
