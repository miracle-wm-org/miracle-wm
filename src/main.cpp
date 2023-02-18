#include <cstdlib>
#include <cstdio>

#include <miral/minimal_window_manager.h>
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

using namespace miral;
using namespace miral::toolkit;

int main(int argc, char const* argv[]) {
    MirRunner runner{argc, argv};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });
 
    ExternalClientLauncher external_client_launcher;
 
    std::string terminal_cmd{"miral-terminal"};

    auto const quit_on_ctrl_alt_bksp = [&](MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return false;
 
            MirInputEvent const* input_event = mir_event_get_input_event(event);
            if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
                return false;
 
            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
            if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
                return false;
 
            MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
            if (!(mods & mir_input_event_modifier_alt) || !(mods & mir_input_event_modifier_ctrl))
                return false;
 
            switch (mir_keyboard_event_keysym(kev))
            {
            case XKB_KEY_BackSpace:
                runner.stop();
                return true;
 
            case XKB_KEY_t:
            case XKB_KEY_T:
                external_client_launcher.launch({terminal_cmd});
                return false;
 
            case XKB_KEY_x:
            case XKB_KEY_X:
                external_client_launcher.launch_using_x11({"xterm"});
                return false;
 
            default:
                return false;
            };
        };

    Keymap config_keymap;
 
    auto run_startup_apps = [&](std::string const& apps)
    {
      for (auto i = begin(apps); i != end(apps); )
      {
          auto const j = find(i, end(apps), ':');
          external_client_launcher.launch(std::vector<std::string>{std::string{i, j}});
          if ((i = j) != end(apps)) ++i;
      }
    };


    return runner.run_with(
        {
            set_window_management_policy<MinimalWindowManager>(),
            WaylandExtensions{},
            X11Support{},
            AppendEventFilter{quit_on_ctrl_alt_bksp},
            config_keymap
        });
    return EXIT_SUCCESS;
}