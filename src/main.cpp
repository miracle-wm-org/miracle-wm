#include <cstdlib>
#include <cstdio>

#include "FloatingWindowManager.hpp"
#include <linux/input-event-codes.h>
#include <miral/set_window_management_policy.h>
#include <miral/display_configuration_option.h>
#include <miral/external_client.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/append_event_filter.h>
#include <miral/internal_client.h>
#include <miral/command_line_option.h>
#include <miral/cursor_theme.h>
#include <miral/keymap.h>
#include <miral/toolkit_event.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <miral/minimal_window_manager.h>

using namespace miral;
using namespace miral::toolkit;

int main(int argc, char const* argv[]) {
    MirRunner runner{argc, argv};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    InternalClientLauncher launcher;
 
    ExternalClientLauncher external_client_launcher;
    WindowManagerOptions window_managers
        {
            add_window_manager_policy<FloatingWindowManagerPolicy>("floating", launcher, shutdown_hook)
        };
 
    std::string terminal_cmd{"xfce4-terminal"};

    auto const onEvent = [&](MirEvent const* event){
        if (mir_event_get_type(event) != mir_event_type_input)
            return false;

        MirInputEvent const* input_event = mir_event_get_input_event(event);
        if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
            return false;

        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
        if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
            return false;

        MirInputEventModifiers modifiers = mir_keyboard_event_modifiers(kev);
        auto const keyEvent = mir_keyboard_event_keysym(kev);
        if ((modifiers & mir_input_event_modifier_meta) && keyEvent == XKB_KEY_Return) {
            external_client_launcher.launch({terminal_cmd});
            return true;
        }

        if (!(modifiers & mir_input_event_modifier_alt) || !(modifiers & mir_input_event_modifier_ctrl))
            return false;

        switch (keyEvent) {
            case XKB_KEY_BackSpace:
                runner.stop();
                return true;
            default:
                return false;
        };
    };

    Keymap config_keymap;


    auto runResult = runner.run_with(
        {
            window_managers,
            WaylandExtensions{},
            X11Support{},
            AppendEventFilter{onEvent},
            config_keymap,
            external_client_launcher
        });

    if (runResult == EXIT_FAILURE) {
        return runResult;
    }

    return EXIT_SUCCESS;
}