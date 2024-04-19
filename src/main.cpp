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

#include "policy.h"
#include "miracle_config.h"
#include "auto_restarting_launcher.h"
#include "renderer.h"

#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/keymap.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>
#include <miral/display_configuration_option.h>
#include <miral/add_init_callback.h>
#include <miral/append_event_filter.h>
#include <miral/custom_renderer.h>
#include <mir/renderer/gl/gl_surface.h>
#include <libnotify/notify.h>
#include <stdlib.h>

using namespace miral;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    ExternalClientLauncher external_client_launcher;
    miracle::AutoRestartingLauncher auto_restarting_launcher(runner, external_client_launcher);
    auto config = std::make_shared<miracle::MiracleConfig>(runner);
    for (auto const& env : config->get_env_variables())
    {
        setenv(env.key.c_str(), env.value.c_str(), 1);
    }

    WindowManagerOptions window_managers
    {
        add_window_manager_policy<miracle::Policy>(
            "tiling", external_client_launcher, runner, config)
    };

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
        {
            window_managers,
            WaylandExtensions{}
                .enable(miral::WaylandExtensions::zwlr_layer_shell_v1)
                .enable(miral::WaylandExtensions::zwlr_foreign_toplevel_manager_v1)
                .enable(miral::WaylandExtensions::zxdg_output_manager_v1)
                .enable(miral::WaylandExtensions::zwp_virtual_keyboard_manager_v1)
                .enable(miral::WaylandExtensions::zwlr_virtual_pointer_manager_v1)
                .enable(miral::WaylandExtensions::zwp_input_method_manager_v2)
                .enable(miral::WaylandExtensions::zwlr_screencopy_manager_v1)
                .enable(miral::WaylandExtensions::ext_session_lock_manager_v1),
            X11Support{}.default_to_enabled(),
            config_keymap,
            external_client_launcher,
            display_configuration_options,
            AddInitCallback(run_startup_apps),
            AppendEventFilter([&config](MirEvent const*)
            {
                config->try_process_change();
                return false;
            }),
            CustomRenderer(
            [&](std::unique_ptr<mir::graphics::gl::OutputSurface> x,
                std::shared_ptr<mir::graphics::GLRenderingProvider> y)
            {
                return std::make_unique<mir::renderer::gl::Renderer>(std::move(y), std::move(x));
            })
        });
}
