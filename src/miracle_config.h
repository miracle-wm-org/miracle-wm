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
    Fullscreen,
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
    [[nodiscard]] int get_gap_size_x() const;
    [[nodiscard]] int get_gap_size_y() const;
    [[nodiscard]] std::vector<std::string> const& get_startup_apps() const;

private:
    static uint parse_modifier(std::string const& stringified_action_key);

    static const uint miracle_input_event_modifier_default = 1 << 18;
    uint primary_modifier = mir_input_event_modifier_meta;
    KeyCommandList key_commands[DefaultKeyCommand::MAX];

    int gap_size_x = 10;
    int gap_size_y = 10;
    std::vector<std::string> startup_apps;
};
}


#endif
