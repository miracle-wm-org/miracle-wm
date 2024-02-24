#define MIR_LOG_COMPONENT "miracle-main"

#include "miracle_window_management_policy.h"
#include "miracle_config.h"
#include "auto_restarting_launcher.h"

#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/internal_client.h>
#include <miral/keymap.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>
#include <miral/display_configuration_option.h>
#include <miral/add_init_callback.h>

using namespace miral;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    InternalClientLauncher internal_client_launcher;
    ExternalClientLauncher external_client_launcher;
    miracle::AutoRestartingLauncher auto_restarting_launcher(runner, external_client_launcher);
    auto config = std::make_shared<miracle::MiracleConfig>(runner);
    WindowManagerOptions window_managers
    {
        add_window_manager_policy<miracle::MiracleWindowManagementPolicy>(
            "tiling", external_client_launcher, internal_client_launcher, runner, config)
    };

    Keymap config_keymap;

    auto const run_startup_apps = [&]()
    {
        for (auto const& app : config->get_startup_apps())
        {
            auto_restarting_launcher.launch(app);
        }
    };

    config->register_listener([&auto_restarting_launcher](miracle::MiracleConfig& new_config)
    {
         auto_restarting_launcher.kill_all();
    });

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
            internal_client_launcher,
            display_configuration_options,
            AddInitCallback(run_startup_apps)
        });
}
