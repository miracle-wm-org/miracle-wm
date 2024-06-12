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

#ifndef MIRACLEWM_MIRACLE_CONFIG_H
#define MIRACLEWM_MIRACLE_CONFIG_H

#include "animation_defintion.h"
#include <atomic>
#include <functional>
#include <glm/glm.hpp>
#include <linux/input.h>
#include <memory>
#include <mir/fd.h>
#include <miral/toolkit_event.h>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace YAML
{
class Node;
}

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
    ResizeUp,
    ResizeDown,
    ResizeLeft,
    ResizeRight,
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
    ToggleFloating,
    TogglePinnedToWorkspace,
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
    bool no_startup_id = false;
};

struct EnvironmentVariable
{
    std::string key;
    std::string value;
};

struct BorderConfig
{
    int size;
    glm::vec4 focus_color;
    glm::vec4 color;
};

class MiracleConfig
{
public:
    explicit MiracleConfig(miral::MirRunner&);
    MiracleConfig(miral::MirRunner&, std::string const&);
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const;
    [[nodiscard]] CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const;
    bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const;
    [[nodiscard]] int get_inner_gaps_x() const;
    [[nodiscard]] int get_inner_gaps_y() const;
    [[nodiscard]] int get_outer_gaps_x() const;
    [[nodiscard]] int get_outer_gaps_y() const;
    [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const;
    [[nodiscard]] std::optional<std::string> const& get_terminal_command() const;
    [[nodiscard]] int get_resize_jump() const;
    [[nodiscard]] std::vector<EnvironmentVariable> const& get_env_variables() const;
    [[nodiscard]] BorderConfig const& get_border_config() const;
    [[nodiscard]] std::array<AnimationDefinition, (int)AnimateableEvent::max> const& get_animation_definitions() const;
    [[nodiscard]] bool are_animations_enabled() const;

    /// Register a listener on configuration change. A lower "priority" number signifies that the
    /// listener should be triggered earlier. A higher priority means later
    int register_listener(std::function<void(miracle::MiracleConfig&)> const&, int priority = 5);
    void unregister_listener(int handle);
    void try_process_change();

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
    void read_animation_definitions(YAML::Node const&);

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
    std::optional<std::string> terminal = "miracle-wm-sensible-terminal";
    std::string desired_terminal = "";
    int resize_jump = 50;
    std::vector<EnvironmentVariable> environment_variables;
    BorderConfig border_config;
    std::atomic<bool> has_changes = false;
    bool animations_enabled = true;
    std::array<AnimationDefinition, (int)AnimateableEvent::max> animation_defintions;
};
}

#endif
