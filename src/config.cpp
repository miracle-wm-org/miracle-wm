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

#define MIR_LOG_COMPONENT "config"

#include "config.h"
#include "config_observer.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glib-2.0/glib.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <mir/log.h>
#include <mir/main_loop.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <miral/runner.h>
#include <sys/inotify.h>

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

FilesystemConfiguration::~FilesystemConfiguration()
{
    if (main_loop)
        main_loop->unregister_fd_handler(this);
}

void FilesystemConfiguration::load(mir::Server& server)
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

    server.add_init_callback([this, config_file_name_option, no_config_option, exec_option, systemd_session_configure_option, &server]
    {
        main_loop = server.the_main_loop();
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
                const auto fs_copyopts = std::filesystem::copy_options::recursive;
                std::filesystem::copy(MIRACLE_DEFAULT_CONFIG_DIR, std::filesystem::path(config_path).parent_path(), fs_copyopts);
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
        options.startup_apps.insert(options.startup_apps.begin(), systemd_app.value());
    }

    // If the user specified an --exec <APP_NAME>, let's add that to the list
    if (exec_app)
    {
        mir::log_info("Miracle will die when the application specified with --exec dies");
        options.startup_apps.push_back(exec_app.value());
    }

    is_loaded_ = true;

    if (main_loop != nullptr)
        _watch(main_loop);
    else
        mir::log_warning("Cannot watch for configuration changes because main_loop is not set");
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
        auto const [config, errors] = load_config(config_path);
        options = config;

        if (!errors.empty())
        {
            for (auto const& error : errors)
                mir::log_error("Configuration parsing error: %s (%s::L%d:%d)",
                    error.message.c_str(),
                    error.filename.c_str(),
                    error.line,
                    error.column);
        }
    }

    observer_registrar->advise_config_changed(*this);
}

void FilesystemConfiguration::_watch(std::shared_ptr<mir::MainLoop> const& main_loop)
{
    if (no_config)
    {
        mir::log_info("No configuration was selected, so the configuration will not be watched");
        return;
    }

    inotify_fd = mir::Fd { inotify_init() };
    file_watch = inotify_add_watch(inotify_fd, config_path.c_str(), IN_MODIFY);
    if (file_watch < 0)
        mir::fatal_error("Unable to watch the config file");

    main_loop->register_fd_handler({ inotify_fd }, this, [&](int file_fd)
    {
        union
        {
            inotify_event event;
            char buffer[sizeof(inotify_event) + NAME_MAX + 1];
        } inotify_buffer;

        if (read(inotify_fd, &inotify_buffer, sizeof(inotify_buffer)) < static_cast<ssize_t>(sizeof(inotify_event)))
            return;

        if (inotify_buffer.event.mask & (IN_MODIFY))
        {
            reload();
        }
    });
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
    if (!options.keymap)
        return std::nullopt;

    return options.keymap->to_string();
}

std::string const& FilesystemConfiguration::get_filename() const
{
    return config_path;
}

MirInputEventModifier FilesystemConfiguration::get_input_event_modifier() const
{
    std::lock_guard lock(mutex);
    return static_cast<MirInputEventModifier>(options.primary_modifier);
}

CustomKeyCommand const*
FilesystemConfiguration::matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const
{
    std::lock_guard lock(mutex);
    // TODO: Copy & paste
    for (auto const& command : options.custom_key_commands)
    {
        if (action != command.action)
            continue;

        auto command_modifiers = process_modifier_internal(command.modifiers);
        if (command_modifiers != modifiers)
            continue;

        if (scan_code == command.key)
            return &command;
    }

    return nullptr;
}

bool FilesystemConfiguration::matches_key_command(
    MirKeyboardAction action,
    int scan_code,
    unsigned int modifiers,
    std::function<bool(DefaultKeyCommand)> const& f) const
{
    constexpr KeyCommand default_key_commands[static_cast<int>(DefaultKeyCommand::MAX)] = {
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_ENTER },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_V     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_H     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_R     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_UP    },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_DOWN  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_LEFT  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_RIGHT },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_UP    },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_DOWN  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_LEFT  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_RIGHT },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_UP    },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_DOWN  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_LEFT  },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_RIGHT },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_Q     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_E     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_F     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_1     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_2     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_3     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_4     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_5     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_6     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_7     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_8     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_9     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_0     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_1     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_2     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_3     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_4     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_5     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_6     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_7     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_8     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_9     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_0     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_SPACE },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_P     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_W     },
        { mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_S     }
    };

    auto const try_run_key_command = [&](KeyCommand const& command, DefaultKeyCommand i)
    {
        if (action != command.action)
            return false;

        auto const command_modifiers = process_modifier(command.modifiers);
        if (command_modifiers != modifiers)
            return false;

        if (scan_code == command.key)
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
        if (try_run_key_command(override, override.default_key_command))
            return true;
    }

    for (size_t i = 0; i < static_cast<int>(DefaultKeyCommand::MAX); i++)
    {
        if (try_run_key_command(default_key_commands[i], static_cast<DefaultKeyCommand>(i)))
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
    return options.animation_definitions[static_cast<size_t>(event)];
}

bool FilesystemConfiguration::are_animations_enabled() const
{
    std::lock_guard lock(mutex);
    return options.animations_enabled;
}

WorkspaceConfig FilesystemConfiguration::get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const
{
    std::lock_guard lock(mutex);
    for (auto const& config : options.workspace_configs)
    {
        if (num && config.num == num.value())
            return config;
        else if (name && config.name == name.value())
            return config;
    }

    return { num, ContainerType::leaf, name };
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
