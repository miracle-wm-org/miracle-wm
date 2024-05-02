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

#define MIR_LOG_COMPONENT "miracle-main"

#include "auto_restarting_launcher.h"
#include "miracle_config.h"
#include "policy.h"

#include <libnotify/notify.h>
#include <miral/add_init_callback.h>
#include <miral/append_event_filter.h>
#include <miral/display_configuration_option.h>
#include <miral/external_client.h>
#include <miral/keymap.h>
#include <miral/runner.h>
#include <miral/wayland_extensions.h>
#include <miral/window_management_options.h>
#include <miral/x11_support.h>
#include <stdlib.h>

using namespace miral;

/// Wraps another miral API so that we can gain access to the underlying Server.
class ServerMiddleman
{
public:
    explicit ServerMiddleman(std::function<void(::mir::Server&)> const& f) :
        f { f }
    {
    }
    void operator()(mir::Server& server) const
    {
        f(server);
    }

private:
    std::function<void(::mir::Server&)> f;
};

int main(int argc, char const* argv[])
{
    MirRunner runner { argc, argv };

    std::function<void()> shutdown_hook { [] { } };
    runner.add_stop_callback([&]
    { shutdown_hook(); });

    ExternalClientLauncher external_client_launcher;
    miracle::AutoRestartingLauncher auto_restarting_launcher(runner, external_client_launcher);
    auto config = std::make_shared<miracle::MiracleConfig>(runner);
    for (auto const& env : config->get_env_variables())
    {
        setenv(env.key.c_str(), env.value.c_str(), 1);
    }

    WindowManagerOptions* options;
    auto window_managers = ServerMiddleman(
        [&](auto& server)
    {
        options = new WindowManagerOptions {
            add_window_manager_policy<miracle::Policy>(
                "tiling", external_client_launcher, runner, config, server)
        };
        (*options)(server);
    });

    Keymap config_keymap;

    auto const run_startup_apps = [&]()
    {
        for (auto const& app : config->get_startup_apps())
        {
            auto_restarting_launcher.launch(app);
        }
    };

    notify_init("miracle-wm");
    return runner.run_with(
        { window_managers,
            WaylandExtensions {}
                .enable(miral::WaylandExtensions::zwlr_layer_shell_v1)
                .enable(miral::WaylandExtensions::zwlr_foreign_toplevel_manager_v1)
                .enable(miral::WaylandExtensions::zxdg_output_manager_v1)
                .enable(miral::WaylandExtensions::zwp_virtual_keyboard_manager_v1)
                .enable(miral::WaylandExtensions::zwlr_virtual_pointer_manager_v1)
                .enable(miral::WaylandExtensions::zwp_input_method_manager_v2)
                .enable(miral::WaylandExtensions::zwlr_screencopy_manager_v1)
                .enable(miral::WaylandExtensions::ext_session_lock_manager_v1),
            X11Support {}.default_to_enabled(),
            config_keymap,
            external_client_launcher,
            display_configuration_options,
            AddInitCallback(run_startup_apps),
            AppendEventFilter([&config](MirEvent const*)
    {
        config->try_process_change();
        return false;
    }) });
}
