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
    SelectWorkspace1,
    SelectWorkspace2,
    SelectWorkspace3,
    SelectWorkspace4,
    SelectWorkspace5,
    SelectWorkspace6,
    SelectWorkspace7,
    SelectWorkspace8,
    SelectWorkspace9,
    SelectWorkspace0,
    MoveToWorkspace1,
    MoveToWorkspace2,
    MoveToWorkspace3,
    MoveToWorkspace4,
    MoveToWorkspace5,
    MoveToWorkspace6,
    MoveToWorkspace7,
    MoveToWorkspace8,
    MoveToWorkspace9,
    MoveToWorkspace0,
    MAX
};


struct KeyCommand
{
    MirKeyboardAction action;
    uint modifiers;
    int key;
};

struct CustomKeyCommand : KeyCommand
{
    std::string command;
};

typedef std::vector<KeyCommand> KeyCommandList;

struct StartupApp
{
    std::string command;
    bool restart_on_death = false;
};

class MiracleConfig
{
public:
    MiracleConfig();
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const;
    CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const;
    [[nodiscard]] DefaultKeyCommand matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const;
    [[nodiscard]] int get_gap_size_x() const;
    [[nodiscard]] int get_gap_size_y() const;
    [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const;

private:
    static uint parse_modifier(std::string const& stringified_action_key);

    static const uint miracle_input_event_modifier_default = 1 << 18;
    uint primary_modifier = mir_input_event_modifier_meta;

    std::vector<CustomKeyCommand> custom_key_commands;
    KeyCommandList key_commands[DefaultKeyCommand::MAX];

    int gap_size_x = 10;
    int gap_size_y = 10;
    std::vector<StartupApp> startup_apps;
};
}


#endif
