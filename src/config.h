/**
Copyright (C) 2025  Matthew Kosarek

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
#include "miracle/cpp/gaps.h"
#include <miracle/cpp/config-cpp.h>
#include <miral/version.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace mir
{
class Server;
}
namespace miracle
{
class ConfigObserverRegistrar;

class Config
{
public:
    virtual ~Config() = default;
    virtual void operator()(mir::Server& server) = 0;
    virtual void reload() = 0;
    [[nodiscard]] virtual std::string const& get_filename() const = 0;
    /// Errors collected during the most recent configuration load. Empty if the
    /// configuration loaded cleanly.
    [[nodiscard]] virtual std::vector<Error> const& get_config_errors() const = 0;
    /// The configured error reporter client (the raw `wm_clients.error_reporter`
    /// value: "default", "disabled", or a path/name of an executable).
    [[nodiscard]] virtual std::string get_error_reporter_client() const = 0;
    /// The configured debug overlay client (the raw `wm_clients.debug_overlay`
    /// value: "default", "disabled", or a path/name of an executable).
    [[nodiscard]] virtual std::string get_debug_overlay_client() const = 0;
    [[nodiscard]] virtual MirInputEventModifier get_input_event_modifier() const = 0;
    [[nodiscard]] virtual CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, uint32_t keysym, unsigned int modifiers) const = 0;
    virtual bool matches_key_command(MirKeyboardAction action, uint32_t keysym, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const = 0;
    [[nodiscard]] virtual Gaps get_inner_gaps() const = 0;
    virtual void override_inner_gaps(Gaps const&) = 0;
    [[nodiscard]] virtual Gaps get_outer_gaps() const = 0;
    virtual void override_outer_gaps(Gaps const&) = 0;
    [[nodiscard]] virtual std::vector<StartupApp> const& get_startup_apps() const = 0;
    [[nodiscard]] virtual std::optional<std::string> const& get_terminal_command() const = 0;
    [[nodiscard]] virtual int get_resize_jump() const = 0;
    [[nodiscard]] virtual std::vector<EnvironmentVariable> const& get_env_variables() const = 0;
    [[nodiscard]] virtual BorderConfig const& get_border_config() const = 0;
    [[nodiscard]] virtual AnimationDefinition const& get_animation_definition(AnimateableEvent event) const = 0;
    [[nodiscard]] virtual bool are_animations_enabled() const = 0;
    [[nodiscard]] virtual WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const = 0;
    [[nodiscard]] virtual LayoutScheme get_default_layout_scheme() const = 0;
    [[nodiscard]] virtual DragAndDropConfiguration drag_and_drop() const = 0;
    [[nodiscard]] virtual uint move_modifier() const = 0;
    [[nodiscard]] virtual uint get_primary_modifier() const = 0;
    [[nodiscard]] virtual uint get_primary_button() const = 0;
    [[nodiscard]] virtual miral::InputConfiguration::Mouse mouse() const = 0;
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
    [[nodiscard]] virtual miral::InputConfiguration::Keyboard keyboard() const = 0;
#endif
    [[nodiscard]] virtual std::optional<std::string> keymap() const = 0;
    [[nodiscard]] virtual MagnifierConfiguration magnifier() const = 0;
    uint process_modifier(uint modifier) const;
    [[nodiscard]] virtual HoverClickConfiguration hover_click() const = 0;
    [[nodiscard]] virtual SimulatedSecondaryClickConfiguration simulated_secondary_click() const = 0;
    [[nodiscard]] virtual OutputFilterConfiguration output_filter() const = 0;
    [[nodiscard]] virtual CursorConfiguration cursor() const = 0;
    [[nodiscard]] virtual SlowKeysConfiguration slow_keys() const = 0;
    [[nodiscard]] virtual StickyKeysConfiguration sticky_keys() const = 0;
    [[nodiscard]] virtual TouchpadConfiguration touchpad() const = 0;
    [[nodiscard]] virtual bool get_workspace_back_and_forth() const = 0;

    /// Register a hook that is called during reload() to allow plugins to
    /// override configuration values. The hook is called after the file config
    /// is loaded and before config-change observers are notified.
    virtual void set_plugin_configure_hook(std::function<PluginConfigData()>&& hook) { }
};

class FilesystemConfiguration : public Config
{
public:
    explicit FilesystemConfiguration(std::shared_ptr<ConfigObserverRegistrar> const&);
    FilesystemConfiguration(std::shared_ptr<ConfigObserverRegistrar> const&, std::string const&, bool load_immediately = false);
    ~FilesystemConfiguration() override;
    FilesystemConfiguration(FilesystemConfiguration const&) = delete;
    auto operator=(FilesystemConfiguration const&) -> FilesystemConfiguration& = delete;

    void operator()(mir::Server& server) override;
    void reload() override;
    [[nodiscard]] std::string const& get_filename() const override;
    [[nodiscard]] std::vector<Error> const& get_config_errors() const override;
    [[nodiscard]] std::string get_error_reporter_client() const override;
    [[nodiscard]] std::string get_debug_overlay_client() const override;
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const override;
    [[nodiscard]] CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, uint32_t keysym, unsigned int modifiers) const override;
    bool matches_key_command(MirKeyboardAction action, uint32_t keysym, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const override;
    [[nodiscard]] Gaps get_inner_gaps() const override;
    void override_inner_gaps(Gaps const&) override;
    [[nodiscard]] Gaps get_outer_gaps() const override;
    void override_outer_gaps(Gaps const&) override;
    [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const override;
    [[nodiscard]] std::optional<std::string> const& get_terminal_command() const override;
    [[nodiscard]] int get_resize_jump() const override;
    [[nodiscard]] std::vector<EnvironmentVariable> const& get_env_variables() const override;
    [[nodiscard]] BorderConfig const& get_border_config() const override;
    [[nodiscard]] AnimationDefinition const& get_animation_definition(AnimateableEvent event) const override;
    [[nodiscard]] bool are_animations_enabled() const override;
    [[nodiscard]] WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const override;
    [[nodiscard]] LayoutScheme get_default_layout_scheme() const override;
    [[nodiscard]] DragAndDropConfiguration drag_and_drop() const override;
    [[nodiscard]] uint move_modifier() const override;
    [[nodiscard]] uint get_primary_modifier() const override;
    [[nodiscard]] uint get_primary_button() const override;
    [[nodiscard]] miral::InputConfiguration::Mouse mouse() const override;
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
    [[nodiscard]] miral::InputConfiguration::Keyboard keyboard() const override;
#endif
    [[nodiscard]] std::optional<std::string> keymap() const override;
    [[nodiscard]] MagnifierConfiguration magnifier() const override;
    [[nodiscard]] HoverClickConfiguration hover_click() const override;
    [[nodiscard]] SimulatedSecondaryClickConfiguration simulated_secondary_click() const override;
    [[nodiscard]] OutputFilterConfiguration output_filter() const override;
    [[nodiscard]] CursorConfiguration cursor() const override;
    [[nodiscard]] SlowKeysConfiguration slow_keys() const override;
    [[nodiscard]] StickyKeysConfiguration sticky_keys() const override;
    [[nodiscard]] TouchpadConfiguration touchpad() const override;
    [[nodiscard]] bool get_workspace_back_and_forth() const override;
    void set_plugin_configure_hook(std::function<PluginConfigData()>&& hook) override;

private:
    uint process_modifier_internal(uint modifier) const;
    void _init(std::optional<StartupApp> const& systemd_app, std::optional<StartupApp> const& exec_app);
    std::shared_ptr<ConfigObserverRegistrar> observer_registrar;
    int next_listener_handle = 0;
    std::string default_config_path;
    std::string config_path;
    bool no_config = false;
    std::mutex mutable mutex;
    bool is_loaded_ = false;
    ConfigData options;
    std::vector<Error> config_errors_;
    std::optional<StartupApp> cached_systemd_app_;
    std::optional<StartupApp> cached_exec_app_;
    std::function<PluginConfigData()> plugin_configure_hook_;
};
}

#endif
