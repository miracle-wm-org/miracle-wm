#include <miral/set_window_management_policy.h>
#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/internal_client.h>
#include <miral/keymap.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>
#include <miral/command_line_option.h>
#include <miral/display_configuration_option.h>
#include <miral/add_init_callback.h>
#include "miracle_window_management_policy.h"
#include "miracle_config.h"

using namespace miral;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    InternalClientLauncher internal_client_launcher;
    ExternalClientLauncher external_client_launcher;
    miracle::MiracleConfig config;
    WindowManagerOptions window_managers
    {
        add_window_manager_policy<miracle::MiracleWindowManagementPolicy>(
            "tiling", external_client_launcher, internal_client_launcher, runner, config)
    };
 
    Keymap config_keymap;

    auto const run_startup_apps = [&]()
    {
        for (auto const& app : config.get_startup_apps())
        {
            external_client_launcher.launch(app);
        }
    };

    return runner.run_with(
        {
            window_managers,
            WaylandExtensions{},
            X11Support{}.default_to_enabled(),
            config_keymap,
            external_client_launcher,
            internal_client_launcher,
            display_configuration_options,
            AddInitCallback(run_startup_apps)
        });
}
