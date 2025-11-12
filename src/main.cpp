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
#include "config_observer.h"
#include "display_config.h"
#include "output_listener.h"
#include "policy.h"
#include "renderer.h"
#include "version.h"
#include "wlr-ouput-management-unstable-v1.h"
#include "wlr-output-management-unstable-v1_wrapper.h"

#include <mir/log.h>
#include <mir/renderer/gl/gl_surface.h>
#include <miral/cursor_scale.h>
#include <miral/custom_renderer.h>
#include <miral/external_client.h>
#include <miral/hover_click.h>
#include <miral/keymap.h>
#include <miral/magnifier.h>
#include <miral/runner.h>
#include <miral/simulated_secondary_click.h>
#include <miral/slow_keys.h>
#include <miral/sticky_keys.h>
#include <miral/wayland_extensions.h>
#include <miral/window_management_options.h>
#include <miral/x11_support.h>

#define PRINT_OPENING_MESSAGE(x) mir::log_info("Welcome to miracle-wm v%s", x);

using namespace miral;

class PolicyLoader
{
public:
    PolicyLoader(ExternalClientLauncher& launcher,
        std::shared_ptr<miracle::Config> const& config,
        std::shared_ptr<miracle::CompositorState> const& compositor_state,
        std::shared_ptr<miracle::OutputListenerMultiplexer> const& output_listener,
        std::shared_ptr<miracle::DisplayConfig> const& display_config,
        std::shared_ptr<miracle::ConfigObserverRegistrar> const& config_observer_registrar,
        Magnifier const& magnifier) :
        launcher(launcher),
        config(config),
        compositor_state(compositor_state),
        output_listener(output_listener),
        display_config(display_config),
        config_observer_registrar(config_observer_registrar),
        magnifier(magnifier)
    {
    }

    void operator()(mir::Server& server)
    {
        config->operator()(server);
        auto policy = add_window_manager_policy<miracle::Policy>(
            "tiling", server, launcher, config, compositor_state, output_listener, display_config, config_observer_registrar, magnifier);
        options = std::make_shared<WindowManagerOptions>(std::initializer_list<WindowManagerOption> { policy });
        options->operator()(server);
    }

private:
    ExternalClientLauncher& launcher;
    std::shared_ptr<miracle::Config> config;
    std::shared_ptr<miracle::CompositorState> compositor_state;
    std::shared_ptr<WindowManagerOptions> options;
    std::shared_ptr<miracle::OutputListenerMultiplexer> output_listener;
    std::shared_ptr<miracle::DisplayConfig> display_config;
    std::shared_ptr<miracle::ConfigObserverRegistrar> config_observer_registrar;
    Magnifier magnifier;
};

int main(int argc, char const* argv[])
{
    PRINT_OPENING_MESSAGE(MIRACLE_VERSION_STRING);
    MirRunner runner { argc, argv };
    auto compositor_state = std::make_shared<miracle::CompositorState>();
    auto output_listener = std::make_shared<miracle::OutputListenerMultiplexer>();
    auto display_config = std::make_shared<miracle::DisplayConfig>();

    ExternalClientLauncher external_client_launcher;
    InputConfiguration input_configuration;
    Magnifier magnifier;
    HoverClick hover_click = HoverClick::disabled();
    SimulatedSecondaryClick simulated_secondary_click = SimulatedSecondaryClick::disabled();
    CursorScale cursor_scale;
    SlowKeys slow_keys = SlowKeys::disabled();
    StickyKeys sticky_keys = StickyKeys::disabled();

    auto config_observer_registrar = std::make_shared<miracle::ConfigObserverRegistrar>();
    auto config = std::make_shared<miracle::FilesystemConfiguration>(config_observer_registrar);
    for (auto const& env : config->get_env_variables())
    {
        setenv(env.key.c_str(), env.value.c_str(), 1);
    }

    class InputConfigurationConfigObserver : public miracle::ConfigObserver
    {
    public:
        InputConfigurationConfigObserver(
            InputConfiguration const& input_configuration,
            Keymap const& keymap, HoverClick const& hover_click,
            SimulatedSecondaryClick const& simulated_secondary_click, CursorScale const& cursor_scale, SlowKeys const& slow_keys, StickyKeys const& sticky_keys) :
            input_configuration(input_configuration),
            keymap(keymap),
            hover_click(hover_click),
            simulated_secondary_click(simulated_secondary_click),
            cursor_scale(cursor_scale),
            slow_keys(slow_keys),
            sticky_keys(sticky_keys)
        {
        }

        void on_config_changed(miracle::Config const& config) override
        {
            input_configuration.mouse(config.mouse());

            // Configure touchpad
            auto touchpad_config = miral::InputConfiguration::Touchpad {};
            auto const miracle_touchpad_config = config.touchpad();
            touchpad_config.disable_while_typing(miracle_touchpad_config.disable_while_typing);
            touchpad_config.disable_with_external_mouse(miracle_touchpad_config.disable_with_external_mouse);
            touchpad_config.acceleration_bias(static_cast<double>(miracle_touchpad_config.acceleration_bias));
            touchpad_config.vscroll_speed(static_cast<double>(miracle_touchpad_config.vscroll_speed));
            touchpad_config.hscroll_speed(static_cast<double>(miracle_touchpad_config.hscroll_speed));
            touchpad_config.tap_to_click(miracle_touchpad_config.tap_to_click);
            touchpad_config.middle_mouse_button_emulation(miracle_touchpad_config.middle_mouse_button_emulation);
            input_configuration.touchpad(touchpad_config);

            // TODO(mattkae): Due to a bug in Mir, it is generally unsafe to reload
            //  the keyboard configuration after the first load, as dead clients (e.g.
            //  swayvnc) can cause the internal listeners in Mir to be fail.
            if (!has_reloaded_keyboard_config)
            {
                has_reloaded_keyboard_config = true;
                input_configuration.keyboard(config.keyboard());
            }

            try
            {
                if (auto const keymap_val = config.keymap())
                    keymap.set_keymap(keymap_val.value());
            }
            catch (std::invalid_argument const& e)
            {
                mir::log_error("Could not set keymap: %s", e.what());
            }

            auto const hover_click_config = config.hover_click();
            hover_click.hover_duration(std::chrono::milliseconds(hover_click_config.hover_duration_milliseconds));
            hover_click.cancel_displacement_threshold(hover_click_config.cancel_displacement_threshold);
            hover_click.reclick_displacement_threshold(hover_click_config.reclick_displacement_threshold);
            if (hover_click_config.enabled)
                hover_click.enable();
            else
                hover_click.disable();

            auto const simulated_secondary_click_config = config.simulated_secondary_click();
            simulated_secondary_click.hold_duration(std::chrono::milliseconds(simulated_secondary_click_config.hold_duration_milliseconds));
            simulated_secondary_click.displacement_threshold(simulated_secondary_click_config.displacement_threshold);
            if (simulated_secondary_click_config.enabled)
                simulated_secondary_click.enable();
            else
                simulated_secondary_click.disable();

            cursor_scale.scale(config.cursor().scale);

            auto const slow_keys_config = config.slow_keys();
            slow_keys.hold_delay(std::chrono::milliseconds(slow_keys_config.hold_delay_milliseconds));
            if (slow_keys_config.enabled)
                slow_keys.enable();
            else
                slow_keys.disable();

            auto const sticky_keys_config = config.sticky_keys();
            sticky_keys.should_disable_if_two_keys_are_pressed_together(sticky_keys_config.should_disable_if_two_keys_are_pressed_together);
            if (sticky_keys_config.enabled)
                sticky_keys.enable();
            else
                sticky_keys.disable();
        }

    private:
        InputConfiguration input_configuration;
        Keymap keymap;
        bool has_reloaded_keyboard_config = false;
        HoverClick hover_click;
        SimulatedSecondaryClick simulated_secondary_click;
        CursorScale cursor_scale;
        SlowKeys slow_keys;
        StickyKeys sticky_keys;
    };

#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 5, 0)
    Keymap keymap = Keymap::system_locale1();
#else
    Keymap keymap;
#endif
    auto const input_config_observer = std::make_shared<InputConfigurationConfigObserver>(input_configuration, keymap, hover_click, simulated_secondary_click, cursor_scale, slow_keys, sticky_keys);
    config_observer_registrar->register_interest(input_config_observer);

    WaylandExtensions wayland_extensions = WaylandExtensions {}
                                               .enable(WaylandExtensions::zwlr_layer_shell_v1)
                                               .enable(WaylandExtensions::zwlr_foreign_toplevel_manager_v1)
                                               .enable(WaylandExtensions::zxdg_output_manager_v1)
                                               .enable(WaylandExtensions::zwp_virtual_keyboard_manager_v1)
                                               .enable(WaylandExtensions::zwlr_virtual_pointer_manager_v1)
                                               .enable(WaylandExtensions::zwp_input_method_manager_v2)
                                               .enable(WaylandExtensions::zwlr_screencopy_manager_v1)
                                               .enable(WaylandExtensions::ext_session_lock_manager_v1);

    for (auto const& extension : { "zwp_pointer_constraints_v1", "zwp_relative_pointer_manager_v1" })
        wayland_extensions.enable(extension);

    wayland_extensions.add_extension({ .name = mir::wayland::OutputManagerV1::interface_name,
        .build = [output_listener = output_listener, display_config = display_config](WaylandExtensions::Context const* context)
    {
        auto extension = std::make_shared<miracle::WlrOutputManagementUnstableV1>(context->display(), display_config);
        output_listener->register_listener(extension);
        return extension;
    } });
    wayland_extensions.enable(mir::wayland::OutputManagerV1::interface_name);

    return runner.run_with(
        { PolicyLoader(external_client_launcher, config, compositor_state, output_listener, display_config, config_observer_registrar, magnifier),
            wayland_extensions,
            X11Support {}.default_to_enabled(),
            keymap,
            *display_config,
            external_client_launcher,
            input_configuration,
            magnifier,
            hover_click,
            simulated_secondary_click,
            cursor_scale,
            slow_keys,
            sticky_keys,
            CustomRenderer([&](std::unique_ptr<mir::graphics::gl::OutputSurface> surface, std::shared_ptr<mir::graphics::GLRenderingProvider> rendering_provider)
    {
        return std::make_unique<miracle::Renderer>(std::move(rendering_provider), std::move(surface), config, compositor_state);
    }) });
}
