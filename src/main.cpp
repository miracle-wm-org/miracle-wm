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

#include "compositor_state.h"
#include "config.h"
#include "display_config.h"
#include "output_listener.h"
#include "policy.h"
#include "renderer.h"
#include "version.h"
#include "wlr-ouput-management-unstable-v1.h"
#include "wlr-output-management-unstable-v1_wrapper.h"

#include <mir/log.h>
#include <mir/renderer/gl/gl_surface.h>
#include <miral/append_event_filter.h>
#include <miral/custom_renderer.h>
#include <miral/external_client.h>
#include <miral/keymap.h>
#include <miral/runner.h>
#include <miral/wayland_extensions.h>
#include <miral/window_management_options.h>
#include <miral/x11_support.h>

#define PRINT_OPENING_MESSAGE(x) mir::log_info("Welcome to miracle-wm v%s", x);

using namespace miral;

class PolicyLoader
{
public:
    PolicyLoader(MirRunner& runner,
        ExternalClientLauncher& launcher,
        std::shared_ptr<miracle::Config> const& config,
        std::shared_ptr<miracle::CompositorState> const& compositor_state,
        std::shared_ptr<miracle::OutputListenerMultiplexer> const& output_listener) :
        runner(runner),
        launcher(launcher),
        config(config),
        compositor_state(compositor_state),
        output_listener(output_listener)
    {
    }

    void operator()(mir::Server& server)
    {
        config->load(server);
        auto policy = add_window_manager_policy<miracle::Policy>(
            "tiling", server, runner, launcher, config, compositor_state, output_listener);
        options = std::make_shared<WindowManagerOptions>(std::initializer_list<WindowManagerOption> { policy });
        options->operator()(server);
    }

private:
    MirRunner& runner;
    ExternalClientLauncher& launcher;
    std::shared_ptr<miracle::Config> config;
    std::shared_ptr<miracle::CompositorState> compositor_state;
    std::shared_ptr<WindowManagerOptions> options;
    std::shared_ptr<miracle::OutputListenerMultiplexer> output_listener;
};

int main(int argc, char const* argv[])
{
    PRINT_OPENING_MESSAGE(MIRACLE_VERSION_STRING);
    MirRunner runner { argc, argv };
    auto compositor_state = std::make_shared<miracle::CompositorState>();
    auto output_listener = std::make_shared<miracle::OutputListenerMultiplexer>();
    auto display_config = std::make_shared<miracle::DisplayConfig>();

    ExternalClientLauncher external_client_launcher;
    auto config = std::make_shared<miracle::FilesystemConfiguration>(runner);
    for (auto const& env : config->get_env_variables())
    {
        setenv(env.key.c_str(), env.value.c_str(), 1);
    }

    Keymap config_keymap;
    WaylandExtensions wayland_extensions = WaylandExtensions {}
                                               .enable(miral::WaylandExtensions::zwlr_layer_shell_v1)
                                               .enable(miral::WaylandExtensions::zwlr_foreign_toplevel_manager_v1)
                                               .enable(miral::WaylandExtensions::zxdg_output_manager_v1)
                                               .enable(miral::WaylandExtensions::zwp_virtual_keyboard_manager_v1)
                                               .enable(miral::WaylandExtensions::zwlr_virtual_pointer_manager_v1)
                                               .enable(miral::WaylandExtensions::zwp_input_method_manager_v2)
                                               .enable(miral::WaylandExtensions::zwlr_screencopy_manager_v1)
                                               .enable(miral::WaylandExtensions::ext_session_lock_manager_v1);

    for (auto const& extension : { "zwp_pointer_constraints_v1", "zwp_relative_pointer_manager_v1" })
        wayland_extensions.enable(extension);

    wayland_extensions.add_extension({ .name = mir::wayland::OutputManagerV1::interface_name,
        .build = [output_listener = output_listener, display_config = display_config](WaylandExtensions::Context const* context)
    {
        auto extension = std::make_shared<miracle::WlrOutputManagementUnstableV1>(context->display(), display_config);
        output_listener->register_listener(extension);
        return extension;
    } });

    return runner.run_with(
        { PolicyLoader(runner, external_client_launcher, config, compositor_state, output_listener),
            wayland_extensions,
            X11Support {}.default_to_enabled(),
            config_keymap,
            *display_config,
            external_client_launcher,
            AppendEventFilter([&config](MirEvent const*)
    {
        config->try_process_change();
        return false;
    }),
            CustomRenderer([&](std::unique_ptr<mir::graphics::gl::OutputSurface> surface, std::shared_ptr<mir::graphics::GLRenderingProvider> rendering_provider)
    {
        return std::make_unique<miracle::Renderer>(std::move(rendering_provider), std::move(surface), config, compositor_state);
    }) });
}
