#ifndef MIRACLEWM_MIRACLE_CONFIG_H
#define MIRACLEWM_MIRACLE_CONFIG_H

#include <string>
#include <miral/toolkit_event.h>
#include <vector>
#include <linux/input.h>
#include <memory>
#include <mir/fd.h>
#include <mutex>
#include <functional>

namespace miral
{
class MirRunner;
class FdHandle;
}

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
    MiracleConfig(miral::MirRunner&);
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const;
    CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const;
    [[nodiscard]] DefaultKeyCommand matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const;
    [[nodiscard]] int get_inner_gaps_x() const;
    [[nodiscard]] int get_inner_gaps_y() const;
    [[nodiscard]] int get_outer_gaps_x() const;
    [[nodiscard]] int get_outer_gaps_y() const;
    [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const;

    /// Register a listener on configuration change. A lower "priority" number signifies that the
    /// listener should be triggered earlier. A higher priority means later
    int register_listener(std::function<void(miracle::MiracleConfig&)> const&, int priority = 5);
    void unregister_listener(int handle);

private:
    struct ChangeListener
    {
        std::function<void(miracle::MiracleConfig&)> listener;
        int priority;
        int handle;
    };

    static uint parse_modifier(std::string const& stringified_action_key);
    void _load();
    void _watch(miral::MirRunner& runner);

    miral::MirRunner& runner;
    int next_listener_handle = 0;
    std::vector<ChangeListener> on_change_listeners;
    std::string config_path;
    mir::Fd inotify_fd;
    std::unique_ptr<miral::FdHandle> watch_handle;
    int file_watch = 0;
    std::mutex mutex;

    static const uint miracle_input_event_modifier_default = 1 << 18;
    uint primary_modifier = mir_input_event_modifier_meta;
    std::vector<CustomKeyCommand> custom_key_commands;
    KeyCommandList key_commands[DefaultKeyCommand::MAX];
    int inner_gaps_x = 10;
    int inner_gaps_y = 10;
    int outer_gaps_x = 10;
    int outer_gaps_y = 10;
    std::vector<StartupApp> startup_apps;

};
}


#endif
