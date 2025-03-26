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

#ifndef MIRACLEWM_CONFIG_H
#define MIRACLEWM_CONFIG_H

#include "container.h"
#include <miracle/miracle-wm-config.h>

#include <atomic>
#include <functional>
#include <linux/input.h>
#include <memory>
#include <mir/fd.h>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace mir
{
class Server;
}

namespace miral
{
class MirRunner;
class FdHandle;
}
namespace miracle
{
class Config
{
public:
    virtual ~Config() = default;
    virtual void load(mir::Server& server) = 0;
    virtual void reload() = 0;
    [[nodiscard]] virtual std::string const& get_filename() const = 0;
    [[nodiscard]] virtual MirInputEventModifier get_input_event_modifier() const = 0;
    [[nodiscard]] virtual CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const = 0;
    virtual bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const = 0;
    [[nodiscard]] virtual int get_inner_gaps_x() const = 0;
    [[nodiscard]] virtual int get_inner_gaps_y() const = 0;
    [[nodiscard]] virtual int get_outer_gaps_x() const = 0;
    [[nodiscard]] virtual int get_outer_gaps_y() const = 0;
    [[nodiscard]] virtual std::vector<StartupApp> const& get_startup_apps() const = 0;
    [[nodiscard]] virtual std::optional<std::string> const& get_terminal_command() const = 0;
    [[nodiscard]] virtual int get_resize_jump() const = 0;
    [[nodiscard]] virtual std::vector<EnvironmentVariable> const& get_env_variables() const = 0;
    [[nodiscard]] virtual BorderConfig const& get_border_config() const = 0;
    [[nodiscard]] virtual std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> const& get_animation_definitions() const = 0;
    [[nodiscard]] virtual bool are_animations_enabled() const = 0;
    [[nodiscard]] virtual WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const = 0;
    [[nodiscard]] virtual LayoutScheme get_default_layout_scheme() const = 0;
    [[nodiscard]] virtual DragAndDropConfiguration drag_and_drop() const = 0;
    [[nodiscard]] virtual uint move_modifier() const = 0;

    virtual int register_listener(std::function<void(Config&)> const&) = 0;
    /// Register a listener on configuration change. A lower "priority" number signifies that the
    /// listener should be triggered earlier. A higher priority means later
    virtual int register_listener(std::function<void(Config&)> const&, int priority) = 0;
    virtual void unregister_listener(int handle) = 0;
    virtual void try_process_change() = 0;
    [[nodiscard]] virtual uint get_primary_modifier() const = 0;
    [[nodiscard]] virtual uint get_primary_button() const = 0;
    uint process_modifier(uint modifier) const;
};

class FilesystemConfiguration : public Config
{
public:
    explicit FilesystemConfiguration(miral::MirRunner&);
    FilesystemConfiguration(miral::MirRunner&, std::string const&, bool load_immediately = false);
    ~FilesystemConfiguration() override = default;
    FilesystemConfiguration(FilesystemConfiguration const&) = delete;
    auto operator=(FilesystemConfiguration const&) -> FilesystemConfiguration& = delete;

    void load(mir::Server& server) override;
    void reload() override;
    [[nodiscard]] std::string const& get_filename() const override;
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const override;
    [[nodiscard]] CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const override;
    bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const override;
    [[nodiscard]] int get_inner_gaps_x() const override;
    [[nodiscard]] int get_inner_gaps_y() const override;
    [[nodiscard]] int get_outer_gaps_x() const override;
    [[nodiscard]] int get_outer_gaps_y() const override;
    [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const override;
    [[nodiscard]] std::optional<std::string> const& get_terminal_command() const override;
    [[nodiscard]] int get_resize_jump() const override;
    [[nodiscard]] std::vector<EnvironmentVariable> const& get_env_variables() const override;
    [[nodiscard]] BorderConfig const& get_border_config() const override;
    [[nodiscard]] std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> const& get_animation_definitions() const override;
    [[nodiscard]] bool are_animations_enabled() const override;
    [[nodiscard]] WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const override;
    [[nodiscard]] LayoutScheme get_default_layout_scheme() const override;
    [[nodiscard]] DragAndDropConfiguration drag_and_drop() const override;
    [[nodiscard]] uint move_modifier() const override;
    int register_listener(std::function<void(Config&)> const&) override;
    int register_listener(std::function<void(Config&)> const&, int priority) override;
    void unregister_listener(int handle) override;
    void try_process_change() override;
    [[nodiscard]] uint get_primary_modifier() const override;
    [[nodiscard]] uint get_primary_button() const override;

private:
    struct ChangeListener
    {
        std::function<void(Config&)> listener;
        int priority;
        int handle;
    };

    void _init(std::optional<StartupApp> const& systemd_app, std::optional<StartupApp> const& exec_app);
    void _watch(miral::MirRunner& runner);
    miral::MirRunner& runner;
    int next_listener_handle = 0;
    std::vector<ChangeListener> on_change_listeners;
    std::string default_config_path;
    std::string config_path;
    bool no_config = false;
    mir::Fd inotify_fd;
    std::unique_ptr<miral::FdHandle> watch_handle;
    int file_watch = 0;
    std::mutex mutex;
    std::atomic<bool> has_changes = false;
    bool is_loaded_ = false;
    ConfigData options;
};
}

#endif
