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

#define MIR_LOG_COMPONENT "config"

#include "config.h"
#include "config_observer.h"
#include "miracle/cpp/file_helpers.h"

#include <filesystem>
#include <fstream>
#include <glib-2.0/glib.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <miral/runner.h>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace miracle;

namespace
{
const char* MIRACLE_DEFAULT_CONFIG_DIR = "/usr/share/miracle-wm/default-config";

}

uint Config::process_modifier(uint modifier) const
{
    if (modifier & miracle_input_event_modifier_default)
        modifier = (modifier & ~miracle_input_event_modifier_default) | static_cast<uint>(get_input_event_modifier());
    return modifier;
}

FilesystemConfiguration::FilesystemConfiguration(std::shared_ptr<ConfigObserverRegistrar> const& observer_registrar) :
    FilesystemConfiguration { observer_registrar, get_config_path() }
{
}

FilesystemConfiguration::FilesystemConfiguration(std::shared_ptr<ConfigObserverRegistrar> const& observer_registrar, std::string const& path, bool load_immediately) :
    observer_registrar { observer_registrar },
    default_config_path { path }
{
    if (load_immediately)
    {
        mir::log_info("FilesystemConfiguration: File is being loaded immediately on construction. "
                      "It is assumed that you are running this inside of a test");
        config_path = default_config_path;
        _init(std::nullopt, std::nullopt);
    }
}

FilesystemConfiguration::~FilesystemConfiguration() = default;

void FilesystemConfiguration::operator()(mir::Server& server)
{
    const char* config_file_name_option = "config";
    server.add_configuration_option(
        config_file_name_option,
        "File path to the miracle-wm yaml configuration file",
        default_config_path);

    const char* no_config_option = "no-config";
    server.add_configuration_option(
        no_config_option,
        "If specified, the configuration file will not be loaded",
        false);

    const char* exec_option = "exec";
    server.add_configuration_option(
        exec_option,
        "Specifies an application that will run when miracle starts. When this application "
        "dies, miracle will also die.",
        "");

    const char* systemd_session_configure_option = "systemd-session-configure";
    server.add_configuration_option(
        systemd_session_configure_option,
        "If specified, this script will setup the systemd session before any apps are run",
        "");

    server.add_pre_init_callback([this, config_file_name_option, no_config_option, exec_option, systemd_session_configure_option, &server]
    {
        auto const server_opts = server.get_options();
        no_config = server_opts->get<bool>(no_config_option);
        config_path = server_opts->get<std::string>(config_file_name_option);
        std::optional<StartupApp> systemd_app = std::nullopt;
        std::optional<StartupApp> exec_app = std::nullopt;

        auto systemd_session_configure = server_opts->get<std::string>(systemd_session_configure_option);
        if (!systemd_session_configure.empty())
            systemd_app = StartupApp { .command = systemd_session_configure };

        if (server_opts->is_set(exec_option))
        {
            auto command = server_opts->get<std::string>(exec_option);
            if (!command.empty())
            {
                exec_app = StartupApp {
                    .command = command,
                    .should_halt_compositor_on_death = true
                };
            }
        }

        _init(systemd_app, exec_app);
    });
}

void FilesystemConfiguration::_init(
    std::optional<StartupApp> const& systemd_app,
    std::optional<StartupApp> const& exec_app)
{
    if (no_config)
    {
        mir::log_info("No configuration option was set, so the file will not be created");
    }
    else
    {
        mir::log_info("Configuration file path is: %s", config_path.c_str());
        if (!std::filesystem::exists(config_path))
        {

            if (!std::filesystem::exists(std::filesystem::path(config_path).parent_path()))
            {
                mir::log_info("Configuration directory path missing, creating it now");
                std::filesystem::create_directories(std::filesystem::path(config_path).parent_path());
            }
            if (std::filesystem::exists(MIRACLE_DEFAULT_CONFIG_DIR))
            {
                mir::log_info("Configuration hierarchy being copied from %s", MIRACLE_DEFAULT_CONFIG_DIR);
                auto const config_dir = get_user_config_dir();
                const auto fs_copyopts = std::filesystem::copy_options::recursive;
                std::filesystem::copy(MIRACLE_DEFAULT_CONFIG_DIR, std::filesystem::path(config_dir), fs_copyopts);
            }
            else
            {
                mir::log_info("Configuration being written blank");
                std::fstream file(config_path, std::ios::out | std::ios::in | std::ios::app);
            }
        }
    }

    reload();

    // If the user specified an --systemd-session-configure <APP_NAME>, let's add that to the list
    if (systemd_app)
    {
        options.startup_apps.value.insert(options.startup_apps.value.begin(), systemd_app.value());
    }

    // If the user specified an --exec <APP_NAME>, let's add that to the list
    if (exec_app)
    {
        mir::log_info("Miracle will die when the application specified with --exec dies");
        options.startup_apps.value.push_back(exec_app.value());
    }

    is_loaded_ = true;
}

namespace
{
ConfigData load_config_recursive(std::string const& path)
{
    auto [config, errors] = load_config(expand_tilde_getenv(path));

    if (!errors.empty())
    {
        for (auto const& error : errors)
            mir::log_error("Configuration parsing error: %s (%s::L%d:%d)",
                error.message.c_str(),
                error.filename.c_str(),
                error.line,
                error.column);
    }

    /// Load the includes provided by this path. Note that this may be recursive, but
    /// miracle will not prevent you from loading erroneously.
    for (auto const& include : *config.includes)
    {
        auto loaded = load_config_recursive(include);
        config = config.merge_with(loaded);
    }

    return config;
}
}

void FilesystemConfiguration::reload()
{
    {
        std::lock_guard lock(mutex);

        if (no_config)
        {
            mir::log_info("No configuration was specified, so the config will not load.");
            options = ConfigData();
            return;
        }

        mir::log_info("Configuration is loading...");
        options = load_config_recursive(config_path);

        // TODO: This might be a hack, I do not know yet.
        //  At any rate, we want users to be confident that their paths will be saved
        //  with the path that they wrote down, so we don't do the tilde resolution
        //  there. At the same time, we don't want this resolution to happen every time
        //  that we access a string, because that seems like an unnecessary overhead.
        //  So it only makes sense to do it at load time. However, this might be annoying
        //  in the long term. We will see!
        if (options.output_filter->shader_path)
            options.output_filter->shader_path = expand_tilde_getenv(options.output_filter->shader_path.value());
    }

    observer_registrar->advise_config_changed(*this);
}

uint FilesystemConfiguration::get_primary_modifier() const
{
    std::lock_guard lock(mutex);
    return options.primary_modifier;
}

uint FilesystemConfiguration::get_primary_button() const
{
    std::lock_guard lock(mutex);
    return options.primary_button;
}

miral::InputConfiguration::Mouse FilesystemConfiguration::mouse() const
{
    std::lock_guard lock(mutex);
    return options.mouse_configuration;
}

#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
miral::InputConfiguration::Keyboard FilesystemConfiguration::keyboard() const
{
    std::lock_guard lock(mutex);
    return options.keyboard_configuration;
}
#endif

std::optional<std::string> FilesystemConfiguration::keymap() const
{
    std::lock_guard lock(mutex);
    if (!*options.keymap)
        return std::nullopt;

    return (*options.keymap)->to_string();
}

HoverClickConfiguration FilesystemConfiguration::hover_click() const
{
    std::lock_guard lock(mutex);
    return options.hover_click;
}

SimulatedSecondaryClickConfiguration FilesystemConfiguration::simulated_secondary_click() const
{
    std::lock_guard lock(mutex);
    return options.simulated_secondary_click;
}

OutputFilterConfiguration FilesystemConfiguration::output_filter() const
{
    std::lock_guard lock(mutex);
    return options.output_filter;
}

CursorConfiguration FilesystemConfiguration::cursor() const
{
    std::lock_guard lock(mutex);
    return options.cursor;
}

SlowKeysConfiguration FilesystemConfiguration::slow_keys() const
{
    std::lock_guard lock(mutex);
    return options.slow_keys;
}

StickyKeysConfiguration FilesystemConfiguration::sticky_keys() const
{
    std::lock_guard lock(mutex);
    return options.sticky_keys;
}

TouchpadConfiguration FilesystemConfiguration::touchpad() const
{
    std::lock_guard lock(mutex);
    return options.touchpad;
}

bool FilesystemConfiguration::get_workspace_back_and_forth() const
{
    std::lock_guard lock(mutex);
    return options.workspace_back_and_forth;
}

MagnifierConfiguration FilesystemConfiguration::magnifier() const
{
    std::lock_guard lock(mutex);
    return options.magnifier;
}

std::string const& FilesystemConfiguration::get_filename() const
{
    return config_path;
}

std::vector<PluginConfiguration> const& FilesystemConfiguration::get_plugins() const
{
    return *options.plugins;
}

MirInputEventModifier FilesystemConfiguration::get_input_event_modifier() const
{
    std::lock_guard lock(mutex);
    return static_cast<MirInputEventModifier>(*options.primary_modifier);
}

CustomKeyCommand const*
FilesystemConfiguration::matches_custom_key_command(MirKeyboardAction action, uint32_t keysym, unsigned int modifiers) const
{
    std::lock_guard lock(mutex);
    // TODO: Copy & paste
    for (auto const& command : *options.custom_key_commands)
    {
        if (action != command.action)
            continue;

        auto command_modifiers = process_modifier_internal(command.modifiers);
        auto effective_modifiers = modifiers;
        if (!(command_modifiers & mir_input_event_modifier_shift))
            effective_modifiers &= ~static_cast<uint>(mir_input_event_modifier_shift);
        if (command_modifiers != effective_modifiers)
            continue;

        if (keysym == command.key)
            return &command;
    }

    return nullptr;
}

bool FilesystemConfiguration::matches_key_command(
    MirKeyboardAction action,
    uint32_t keysym,
    unsigned int modifiers,
    std::function<bool(DefaultKeyCommand)> const& f) const
{
    struct KeyCommand
    {
        MirKeyboardAction action;
        uint modifiers;
        uint key;
    };

    constexpr KeyCommand default_key_commands[static_cast<int>(DefaultKeyCommand::MAX)] = {
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Return      },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_v           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_h           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_r           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Up          },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Down        },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Left        },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Right       },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         XKB_KEY_Up          },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         XKB_KEY_Down        },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         XKB_KEY_Left        },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         XKB_KEY_Right       },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Up          },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Down        },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Left        },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Right       },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Q           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_E           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_f           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_1           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_2           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_3           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_4           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_5           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_6           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_7           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_8           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_9           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_0           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_exclam      },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_at          },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_numbersign  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_dollar      },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_percent     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_asciicircum },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_ampersand   },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_asterisk    },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_parenleft   },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_parenright  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_space       },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_P           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_w           },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_s           },
        { // MagnifierOn
            mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_plus        },
        { // MagnifierOff
            mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_Escape      },
        { // MagnifierIncreaseSize
            mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_plus        },
        { // MagnifierDecreaseSize
            mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_underscore  },
        { // MagnifierIncreaseScale
            mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_equal       },
        { // MagnifierDecreaseScale
            mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         XKB_KEY_minus       },
        { // ReloadConfig
            mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         XKB_KEY_R           }
    };

    auto const try_run_key_command = [&](MirKeyboardAction in_action, uint in_modifiers, uint in_key, DefaultKeyCommand i)
    {
        if (action != in_action)
            return false;

        auto const command_modifiers = process_modifier(in_modifiers);
        auto effective_modifiers = modifiers;
        if (!(command_modifiers & mir_input_event_modifier_shift))
            effective_modifiers &= ~static_cast<uint>(mir_input_event_modifier_shift);
        if (command_modifiers != effective_modifiers)
            return false;

        if (keysym == in_key)
        {
            if (f(i))
                return true;
        }

        return false;
    };

    // TODO: This copy may be somewhat expensive. This helps avoid
    //  a deadlock for now, but at a cost.
    std::vector<BuiltInKeyCommandOverride> overrides;
    {
        std::lock_guard lock(mutex);
        overrides = options.built_in_key_command_overrides;
    }

    for (auto& override : overrides)
    {
        if (try_run_key_command(override.action, override.modifiers, override.key, override.default_key_command))
            return true;
    }

    for (size_t i = 0; i < static_cast<int>(DefaultKeyCommand::MAX); i++)
    {
        if (try_run_key_command(default_key_commands[i].action, default_key_commands[i].modifiers, default_key_commands[i].key, static_cast<DefaultKeyCommand>(i)))
            return true;
    }

    return false;
}

Gaps FilesystemConfiguration::get_inner_gaps() const
{
    {
        std::lock_guard lock(mutex);
        return options.inner_gaps;
    }
}

void FilesystemConfiguration::override_inner_gaps(Gaps const& gaps)
{
    {
        std::lock_guard lock(mutex);
        options.inner_gaps = gaps;
    }

    observer_registrar->advise_config_changed(*this);
}

Gaps FilesystemConfiguration::get_outer_gaps() const
{
    std::lock_guard lock(mutex);
    return options.outer_gaps;
}

void FilesystemConfiguration::override_outer_gaps(Gaps const& gaps)
{
    {
        std::lock_guard lock(mutex);
        options.outer_gaps = gaps;
    }

    observer_registrar->advise_config_changed(*this);
}

const std::vector<StartupApp>& FilesystemConfiguration::get_startup_apps() const
{
    std::lock_guard lock(mutex);
    return options.startup_apps;
}

std::optional<std::string> const& FilesystemConfiguration::get_terminal_command() const
{
    std::lock_guard lock(mutex);
    return options.terminal;
}

int FilesystemConfiguration::get_resize_jump() const
{
    std::lock_guard lock(mutex);
    return options.resize_jump;
}

std::vector<EnvironmentVariable> const& FilesystemConfiguration::get_env_variables() const
{
    std::lock_guard lock(mutex);
    return options.environment_variables;
}

BorderConfig const& FilesystemConfiguration::get_border_config() const
{
    std::lock_guard lock(mutex);
    return options.border_config;
}

AnimationDefinition const& FilesystemConfiguration::get_animation_definition(AnimateableEvent event) const
{
    std::lock_guard lock(mutex);
    return (*options.animation_definitions)[static_cast<size_t>(event)];
}

bool FilesystemConfiguration::are_animations_enabled() const
{
    std::lock_guard lock(mutex);
    return options.animations_enabled;
}

WorkspaceConfig FilesystemConfiguration::get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const
{
    std::lock_guard lock(mutex);
    for (auto const& config : *options.workspace_configs)
    {
        if (num && config.num == num.value())
            return config;
        else if (name && config.name == name.value())
            return config;
    }

    return { num, name };
}

LayoutScheme FilesystemConfiguration::get_default_layout_scheme() const
{
    std::lock_guard lock(mutex);
    return LayoutScheme::horizontal;
}

DragAndDropConfiguration FilesystemConfiguration::drag_and_drop() const
{
    std::lock_guard lock(mutex);
    return options.drag_and_drop;
}

uint FilesystemConfiguration::move_modifier() const
{
    std::lock_guard lock(mutex);
    return options.move_modifier;
}

uint FilesystemConfiguration::process_modifier_internal(uint modifier) const
{
    if (modifier & miracle_input_event_modifier_default)
        modifier = (modifier & ~miracle_input_event_modifier_default) | options.primary_modifier;
    return modifier;
}
