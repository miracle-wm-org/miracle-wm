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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glib-2.0/glib.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <miral/runner.h>
#include <sstream>
#include <sys/inotify.h>

using namespace miracle;

namespace
{
const char* MIRACLE_DEFAULT_CONFIG_DIR = "/usr/share/miracle-wm/default-config";

int program_exists(std::string const& name)
{
    std::stringstream out;
    out << "command -v " << name << " > /dev/null 2>&1";
    return !system(out.str().c_str());
}

std::string create_default_configuration_path()
{
    std::stringstream config_path_stream;
    config_path_stream << g_get_user_config_dir();
    config_path_stream << "/miracle-wm.yaml";
    return config_path_stream.str();
}

std::optional<MirKeyboardAction> from_string_keyboard_action(std::string const& action)
{
    if (action == "up")
        return MirKeyboardAction::mir_keyboard_action_up;
    else if (action == "down")
        return MirKeyboardAction::mir_keyboard_action_down;
    else if (action == "repeat")
        return MirKeyboardAction::mir_keyboard_action_repeat;
    else if (action == "modifiers")
        return MirKeyboardAction::mir_keyboard_action_modifiers;
    else
        return std::nullopt;
}

}

uint Config::process_modifier(uint modifier) const
{
    if (modifier & miracle_input_event_modifier_default)
        modifier = modifier & ~miracle_input_event_modifier_default | get_input_event_modifier();
    return modifier;
}

FilesystemConfiguration::FilesystemConfiguration(miral::MirRunner& runner) :
    FilesystemConfiguration { runner, create_default_configuration_path() }
{
}

FilesystemConfiguration::FilesystemConfiguration(
    miral::MirRunner& runner, std::string const& path, bool load_immediately) :
    runner { runner },
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
    _watch(runner);
}

void FilesystemConfiguration::reload()
{
    std::lock_guard<std::mutex> lock(mutex);

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

void FilesystemConfiguration::_watch(miral::MirRunner& runner)
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

    watch_handle = runner.register_fd_handler(inotify_fd, [&](int file_fd)
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
            has_changes = true;
        }
    });
}

void FilesystemConfiguration::try_process_change()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!has_changes)
        return;

    has_changes = false;
    for (auto const& on_change : on_change_listeners)
    {
        on_change.listener(*this);
    }
}

uint FilesystemConfiguration::get_primary_modifier() const
{
    return options.primary_modifier;
}

uint FilesystemConfiguration::get_primary_button() const
{
    return options.primary_button;
}

std::string const& FilesystemConfiguration::get_filename() const
{
    return config_path;
}

MirInputEventModifier FilesystemConfiguration::get_input_event_modifier() const
{
    return static_cast<MirInputEventModifier>(options.primary_modifier);
}

CustomKeyCommand const*
FilesystemConfiguration::matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const
{
    // TODO: Copy & paste
    for (auto const& command : options.custom_key_commands)
    {
        if (action != command.action)
            continue;

        auto command_modifiers = process_modifier(command.modifiers);
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
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_ENTER },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_V     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_H     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_R     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_UP    },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_DOWN  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_LEFT  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_RIGHT },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_UP    },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_DOWN  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_LEFT  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_RIGHT },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_UP    },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_DOWN  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_LEFT  },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_RIGHT },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_Q     },
        { MirKeyboardAction::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_E     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_F     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_1     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_2     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_3     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_4     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_5     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_6     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_7     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_8     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_9     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_0     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_1     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_2     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_3     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_4     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_5     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_6     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_7     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_8     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_9     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_0     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_SPACE },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default | mir_input_event_modifier_shift,
         KEY_P     },
        { MirKeyboardAction ::mir_keyboard_action_down,
         miracle_input_event_modifier_default,
         KEY_W     },
        { MirKeyboardAction ::mir_keyboard_action_down,
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

    for (size_t i = 0; i < options.built_in_key_command_overrides.size(); i++)
    {
        if (try_run_key_command(options.built_in_key_command_overrides[i], options.built_in_key_command_overrides[i].default_key_command))
            return true;
    }

    for (size_t i = 0; i < static_cast<int>(DefaultKeyCommand::MAX); i++)
    {
        if (try_run_key_command(default_key_commands[i], static_cast<DefaultKeyCommand>(i)))
            return true;
    }

    return false;
}

int FilesystemConfiguration::get_inner_gaps_x() const
{
    return options.inner_gaps_x;
}

int FilesystemConfiguration::get_inner_gaps_y() const
{
    return options.inner_gaps_y;
}

int FilesystemConfiguration::get_outer_gaps_x() const
{
    return options.outer_gaps_x;
}

int FilesystemConfiguration::get_outer_gaps_y() const
{
    return options.outer_gaps_y;
}

const std::vector<StartupApp>& FilesystemConfiguration::get_startup_apps() const
{
    return options.startup_apps;
}

int FilesystemConfiguration::register_listener(std::function<void(Config&)> const& func)
{
    return register_listener(func, 5);
}

int FilesystemConfiguration::register_listener(std::function<void(Config&)> const& func, int priority)
{
    int handle = next_listener_handle++;

    for (auto it = on_change_listeners.begin(); it != on_change_listeners.end(); it++)
    {
        if (it->priority >= priority)
        {
            on_change_listeners.insert(it, { func, priority, handle });
            return handle;
        }
    }

    on_change_listeners.push_back({ func, priority, handle });
    return handle;
}

void FilesystemConfiguration::unregister_listener(int handle)
{
    for (auto it = on_change_listeners.begin(); it != on_change_listeners.end(); it++)
    {
        if (it->handle == handle)
        {
            on_change_listeners.erase(it);
            return;
        }
    }
}

std::optional<std::string> const& FilesystemConfiguration::get_terminal_command() const
{
    return options.terminal;
}

int FilesystemConfiguration::get_resize_jump() const
{
    return options.resize_jump;
}

std::vector<EnvironmentVariable> const& FilesystemConfiguration::get_env_variables() const
{
    return options.environment_variables;
}

BorderConfig const& FilesystemConfiguration::get_border_config() const
{
    return options.border_config;
}

std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> const& FilesystemConfiguration::get_animation_definitions() const
{
    return options.animation_definitions;
}

bool FilesystemConfiguration::are_animations_enabled() const
{
    return options.animations_enabled;
}

WorkspaceConfig FilesystemConfiguration::get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const
{
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
    return LayoutScheme::horizontal;
}

DragAndDropConfiguration FilesystemConfiguration::drag_and_drop() const
{
    return options.drag_and_drop;
}

uint FilesystemConfiguration::move_modifier() const
{
    return options.move_modifier;
}