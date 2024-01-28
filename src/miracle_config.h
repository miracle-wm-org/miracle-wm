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
    static const int  miracle_input_event_modifier_default = 1 << 18;
    MirInputEventModifier primary_modifier = mir_input_event_modifier_meta;
    KeyCommandList key_commands[DefaultKeyCommand::MAX] = {
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                KEY_ENTER
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                KEY_V
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                KEY_H
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                KEY_R
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default | mir_input_event_modifier_shift,
                KEY_UP
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default | mir_input_event_modifier_shift,
                KEY_DOWN
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default | mir_input_event_modifier_shift,
                KEY_LEFT
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default | mir_input_event_modifier_shift,
                KEY_RIGHT
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                    KEY_UP
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                    KEY_DOWN
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                    KEY_LEFT
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default,
                    KEY_RIGHT
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default | mir_input_event_modifier_shift,
                KEY_Q
            }
        },
        {
            {
                MirKeyboardAction::mir_keyboard_action_down,
                miracle_input_event_modifier_default | mir_input_event_modifier_shift,
                KEY_E
            }
        },
    };
};
}


#endif
