
#include <miral/set_window_management_policy.h>
#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/internal_client.h>
#include <miral/keymap.h>
#include <miral/toolkit_event.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>
#include "mirie_window_management_policy.h"

//using namespace miral;
//using namespace miral::toolkit;

#include "tile_manager.h"
#include "tile.h"
#include <memory>

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    InternalClientLauncher launcher;
    ExternalClientLauncher external_client_launcher;
    WindowManagerOptions window_managers
    {
        add_window_manager_policy<mirie::MirieWindowManagementPolicy>("tiling", external_client_launcher)
    };
 
    Keymap config_keymap;

    return runner.run_with(
        {
            window_managers,
            WaylandExtensions{},
            X11Support{},
            config_keymap,
            external_client_launcher
        });
}
