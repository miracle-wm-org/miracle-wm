/**
Copyright (C) 2025  Matthew Kosarek

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

#include "abstract_output.h"
#include <memory>
#define MIR_LOG_COMPONENT "miracle"

#include "abstract_workspace.h"
#include "animator_loop.h"
#include "binding_event.h"
#include "carousel_controller.h"
#include "carousel_scene_override.h"
#include "config.h"
#include "config_observer.h"
#include "constants.h"
#include "container_listener.h"
#include "debug_overlay_controller.h"
#include "dying_surface_manager.h"
#include "freestyle_window_container.h"
#include "internal_shell_application_spawner.h"
#include "leaf_container.h"
#include "magnifier_wrapper.h"
#include "output_factory.h"
#include "output_listener.h"
#include "output_manager.h"
#include "parent_container.h"
#include "plugin_bridge.h"
#include "plugin_managed_container.h"
#include "plugin_manager.h"
#include "policy.h"
#include "shell_application_manager.h"
#include "shell_component_container.h"
#include "window_container.h"
#include "window_observer.h"
#include "workspace_manager.h"

#include "plugin_helpers.h"
#include <iostream>
#include <map>
#include <mir/geometry/rectangle.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir_toolkit/events/enums.h>
#include <miral/toolkit_event.h>
#include <miral/window_specification.h>
#include <miral/zone.h>
#include <mutex>
#include <unordered_map>

using namespace miracle;

namespace
{
class MirRunnerCommandControllerInterface : public CommandControllerInterface
{
public:
    explicit MirRunnerCommandControllerInterface(std::shared_ptr<mir::MainLoop> const& main_loop) :
        main_loop { main_loop }
    {
    }

    void quit() override
    {
        main_loop->stop();
    }

private:
    std::shared_ptr<mir::MainLoop> main_loop;
};

}

class Policy::Self : public virtual WorkspaceObserver,
                     public virtual ContainerListener,
                     public virtual ConfigObserver
{
public:
    explicit Self(Policy& policy, std::shared_ptr<mir::ServerActionQueue> const& queue) :
        policy { policy },
        queue { queue }
    {
    }

    void on_workspace_created(uint32_t id) override
    {
        queue->enqueue(this, [id = id, pm = policy.plugin_manager]
        {
            pm->workspace_created(id);
        });
    }
    void on_workspace_removed(uint32_t id) override
    {
        queue->enqueue(this, [id = id, pm = policy.plugin_manager]
        {
            pm->workspace_removed(id);
        });
    }
    void on_workspace_empty(uint32_t) override { }
    void on_workspace_focused(std::optional<uint32_t> old, uint32_t next) override
    {
        if (old)
        {
            auto const& last_workspace = policy.workspace_manager->workspace(old.value());
            auto const& next_workspace = policy.workspace_manager->workspace(next);
            if (!next_workspace)
            {
                mir::log_error("Policy::Self::on_focused missing next workspace");
                return;
            }

            if (last_workspace && last_workspace->get_output() != next_workspace->get_output())
                policy.command_controller->move_cursor_to_output(*next_workspace->get_output());
        }

        queue->enqueue(this, [old = old, next = next, pm = policy.plugin_manager]
        {
            pm->workspace_focused(old, next);
        });
    }
    void on_workspace_renamed(uint32_t) override { }
    void on_workspace_area_changed(uint32_t id) override
    {
        queue->enqueue(this, [id = id, pm = policy.plugin_manager]
        {
            pm->workspace_area_changed(id);
        });
    }

    void on_container_fullscreen(Container const& container) override
    {
        policy.window_observer_registrar->advise_window_fullscreen(container);
    }

    void on_container_moved(Container const& container) override
    {
        policy.window_observer_registrar->advise_window_move(container);
    }

    void on_container_float(Container const& container) override
    {
        policy.window_observer_registrar->advise_window_float(container);
    }

    void on_container_mark(Container const& container) override
    {
        policy.window_observer_registrar->advise_window_marked(container);
    }

    void on_container_workspace_changed(Container const& container) override
    {
        if (!container.window().has_value())
            return;
        auto const& window_info = policy.tools.info_for(container.window().value());
        auto const workspace = container.get_workspace();
        if (!workspace)
            return;
        queue->enqueue(this, [window_info, workspace, pm = policy.plugin_manager]
        {
            pm->window_workspace_changed(window_info, workspace->id());
        });
    }

    void on_config_changed(Config const& config) override
    {
        for (auto const& output : policy.output_manager->outputs())
        {
            for (auto const& workspace : output->get_workspaces())
                workspace->recalculate_area();
        }

        if (!has_loaded_once)
        {
            for (auto const& app : policy.config->get_startup_apps())
                policy.launcher->launch(app);

            if (config.magnifier().enabled)
                policy.magnifier->enable();
            else
                policy.magnifier->disable();
        }

        policy.magnifier->set_scale(config.magnifier().scale);
        policy.magnifier->set_size(config.magnifier().width, config.magnifier().height);
        has_loaded_once = true;
    }

    void on_plugins_changed(std::vector<PluginConfiguration> const& plugins) override
    {
        update_plugins(
            plugins,
            loaded_plugins,
            [this](std::string const& path, std::string const& userdata_json)
        {
            return policy.plugin_manager->load_wasm_module(path, userdata_json);
        },
            [this](PluginHandle handle)
        {
            policy.plugin_manager->unload_wasm_module(handle);
        });
    }

    Policy& policy;
    std::shared_ptr<mir::ServerActionQueue> queue;
    bool has_loaded_once = false;
    std::map<std::string, LoadedPlugin> loaded_plugins;
};

Policy::Policy(
    miral::WindowManagerTools const& tools,
    mir::Server& server,
    miral::ExternalClientLauncher& external_client_launcher,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<OutputListenerMultiplexer> const& output_listener,
    std::shared_ptr<DisplayConfig> const& display_config,
    std::shared_ptr<ConfigObserverRegistrar> const& config_observer_registrar,
    miral::Magnifier const& magnifier,
    std::shared_ptr<SamplerRegistry> const& sampler_registry) :
    tools { tools },
    config { config },
    state { state },
    output_listener { output_listener },
    config_observer_registrar { config_observer_registrar },
    animator(std::make_shared<Animator>()),
    plugin_manager(load_plugin_manager()),
    window_id_map_(std::make_shared<WindowIdMap>()),
    application_id_map_(std::make_shared<ApplicationIdMap>()),
    window_controller(std::make_shared<WindowManagerToolsWindowController>(
        tools, animator, plugin_manager, server.the_main_loop(), state, config, window_id_map_)),
    launcher { std::make_shared<AutoRestartingLauncher>(server, external_client_launcher) },
    workspace_observer_registrar(std::make_shared<WorkspaceObserverRegistrar>()),
    mode_observer_registrar(std::make_shared<ModeObserverRegistrar>()),
    shell_application_manager(std::make_shared<ShellApplicationManager>(std::make_unique<InternalShellApplicationSpawner>(server))),
    output_manager(std::make_shared<OutputManager>(
        std::make_unique<MiralOutputFactory>(
            shell_application_manager,
            state,
            config,
            window_controller,
            animator,
            display_config,
            plugin_manager))),
    workspace_manager(std::make_shared<WorkspaceManager>(workspace_observer_registrar, config, output_manager)),
    self(std::make_shared<Self>(*this, server.the_main_loop())),
    scratchpad_(std::make_shared<Scratchpad>(window_controller, output_manager)),
    window_observer_registrar(std::make_shared<WindowObserverRegistrar>()),
    command_controller(std::make_shared<CommandController>(
        config, state, window_controller,
        workspace_manager, mode_observer_registrar,
        std::make_unique<MirRunnerCommandControllerInterface>(server.the_main_loop()), scratchpad_, output_manager, animator, shell_application_manager, window_observer_registrar, self, plugin_manager)),
    drag_and_drop_service(std::make_unique<DragAndDropService>(command_controller, config, output_manager)),
    move_service(std::make_unique<MoveService>(command_controller, config, output_manager)),
    resize_service(std::make_unique<ResizeService>(command_controller, config, state, output_manager)),
    debug_overlay_controller(std::make_shared<DebugOverlayController>(external_client_launcher, config)),
    ipc_command_executor(std::make_shared<IpcCommandExecutor>(command_controller, launcher, debug_overlay_controller)),
    ipc_connection_manager(std::make_shared<IpcConnectionManager>(
        server.the_main_loop(),
        command_controller,
        ipc_command_executor,
        config,
        window_controller,
        plugin_manager)),
    binding_event_listener_ { ipc_connection_manager.get() },
    animator_loop(std::make_unique<ThreadedAnimatorLoop>(animator)),
    main_loop_(server.the_main_loop()),
    dying_surface_manager(std::make_unique<DyingSurfaceManager>(
        server.the_surface_stack(),
        state,
        config,
        animator,
        plugin_manager,
        window_controller,
        output_manager)),
    magnifier(std::make_unique<MagnifierWrapper>(magnifier)),
    carousel_controller(std::make_unique<CarouselController>(state, output_manager, animator, window_controller, config, command_controller))
{
    plugin_manager->initialize(std::make_unique<PluginBridge>(output_manager, window_controller, workspace_manager, state, window_id_map_, application_id_map_, animator, server.the_main_loop(), sampler_registry,
        [icm = ipc_connection_manager](std::string const& ns, std::string const& payload)
    {
        icm->on_plugin_event(ns, payload);
    }));
    config->set_plugin_configure_hook([pm = plugin_manager]()
    {
        return pm->configure();
    });
    workspace_observer_registrar->register_interest(ipc_connection_manager);
    workspace_observer_registrar->register_interest(self);
    mode_observer_registrar->register_interest(ipc_connection_manager);
    window_observer_registrar->register_interest(ipc_connection_manager);
    config_observer_registrar->register_interest(ipc_connection_manager);
    output_listener->register_listener(ipc_connection_manager);
    config_observer_registrar->register_interest(self);
    config_observer_registrar->register_interest(debug_overlay_controller);
    animator_loop->start();

    server.set_exception_handler([ipc_connection_manager = ipc_connection_manager]
    {
        // If an exception occurs, send out the shutdown event to all clients
        // in case we're crashing. This will ensure that they teardown their
        // systemd sessions appropriately, if necessary.
        ipc_connection_manager->on_shutdown();
    });
}

Policy::~Policy()
{
    ipc_connection_manager->on_shutdown();
    animator_loop->stop();
    workspace_observer_registrar->unregister_interest(ipc_connection_manager.get());
    workspace_observer_registrar->unregister_interest(self.get());
    mode_observer_registrar->unregister_interest(ipc_connection_manager.get());
    config_observer_registrar->unregister_interest(ipc_connection_manager.get());
    output_listener->unregister_listener(ipc_connection_manager);
    config_observer_registrar->unregister_interest(self.get());
}

bool Policy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;
    auto const keysym = miral::toolkit::mir_keyboard_event_keysym(event);

    if (auto const scene_override = state->scene_override_manager()->try_resolve())
    {
        carousel_controller->break_tap();
        scene_override->handle_keyboard_event(event);
        return true;
    }

    carousel_controller->handle_keyboard_event(event, modifiers);

    if (plugin_manager->handle_keyboard_event(*event))
        return true;

    if (auto const custom_key_command = config->matches_custom_key_command(action, keysym, modifiers))
    {
        BindingEvent const binding_event(
            BINDING_MODE_STRINGS[static_cast<size_t>(state->mode())],
            custom_key_command->command,
            modifiers,
            keysym,
            BindingEventType::keyboard);
        ipc_connection_manager->on_binding_event(binding_event);

        launcher->launch({ custom_key_command->command });
        return true;
    }

    if (config->matches_key_command(action, keysym, modifiers, [&](DefaultKeyCommand key_command)
    {
        if (key_command == DefaultKeyCommand::MAX)
            return false;

        auto const handled = [&]() -> bool
        {
            switch (key_command)
            {
            case DefaultKeyCommand::Terminal:
            {
                if (auto const terminal_command = config->get_terminal_command())
                    launcher->launch({ terminal_command.value() });
                return true;
            }
            case DefaultKeyCommand::RequestVertical:
                return command_controller->try_request_vertical(empty_scope);
            case DefaultKeyCommand::RequestHorizontal:
                return command_controller->try_request_horizontal(empty_scope);
            case DefaultKeyCommand::ToggleResize:
                command_controller->try_toggle_resize_mode();
                return true;
            case DefaultKeyCommand::ResizeUp:
                return state->mode() != WindowManagerMode::normal && command_controller->try_resize(Direction::up, config->get_resize_jump(), empty_scope);
            case DefaultKeyCommand::ResizeDown:
                return state->mode() != WindowManagerMode::normal && command_controller->try_resize(Direction::down, config->get_resize_jump(), empty_scope);
            case DefaultKeyCommand::ResizeLeft:
                return state->mode() != WindowManagerMode::normal && command_controller->try_resize(Direction::left, config->get_resize_jump(), empty_scope);
            case DefaultKeyCommand::ResizeRight:
                return state->mode() != WindowManagerMode::normal && command_controller->try_resize(Direction::right, config->get_resize_jump(), empty_scope);
            case DefaultKeyCommand::MoveUp:
                return command_controller->try_move_by_direction(Direction::up, empty_scope);
            case DefaultKeyCommand::MoveDown:
                return command_controller->try_move_by_direction(Direction::down, empty_scope);
            case DefaultKeyCommand::MoveLeft:
                return command_controller->try_move_by_direction(Direction::left, empty_scope);
            case DefaultKeyCommand::MoveRight:
                return command_controller->try_move_by_direction(Direction::right, empty_scope);
            case DefaultKeyCommand::SelectUp:
                return command_controller->try_select(Direction::up, empty_scope);
            case DefaultKeyCommand::SelectDown:
                return command_controller->try_select(Direction::down, empty_scope);
            case DefaultKeyCommand::SelectLeft:
                return command_controller->try_select(Direction::left, empty_scope);
            case DefaultKeyCommand::SelectRight:
                return command_controller->try_select(Direction::right, empty_scope);
            case DefaultKeyCommand::QuitActiveWindow:
                return command_controller->try_close_window(empty_scope);
            case DefaultKeyCommand::QuitCompositor:
                return command_controller->quit();
            case DefaultKeyCommand::Fullscreen:
                return command_controller->try_toggle_fullscreen(empty_scope);
            case DefaultKeyCommand::SelectWorkspace1:
                return command_controller->select_workspace(1, true);
            case DefaultKeyCommand::SelectWorkspace2:
                return command_controller->select_workspace(2, true);
            case DefaultKeyCommand::SelectWorkspace3:
                return command_controller->select_workspace(3, true);
            case DefaultKeyCommand::SelectWorkspace4:
                return command_controller->select_workspace(4, true);
            case DefaultKeyCommand::SelectWorkspace5:
                return command_controller->select_workspace(5, true);
            case DefaultKeyCommand::SelectWorkspace6:
                return command_controller->select_workspace(6, true);
            case DefaultKeyCommand::SelectWorkspace7:
                return command_controller->select_workspace(7, true);
            case DefaultKeyCommand::SelectWorkspace8:
                return command_controller->select_workspace(8, true);
            case DefaultKeyCommand::SelectWorkspace9:
                return command_controller->select_workspace(9, true);
            case DefaultKeyCommand::SelectWorkspace0:
                return command_controller->select_workspace(0, true);
            case DefaultKeyCommand::MoveToWorkspace1:
                return command_controller->try_move_to_workspace(empty_scope, 1, true);
            case DefaultKeyCommand::MoveToWorkspace2:
                return command_controller->try_move_to_workspace(empty_scope, 2, true);
            case DefaultKeyCommand::MoveToWorkspace3:
                return command_controller->try_move_to_workspace(empty_scope, 3, true);
            case DefaultKeyCommand::MoveToWorkspace4:
                return command_controller->try_move_to_workspace(empty_scope, 4, true);
            case DefaultKeyCommand::MoveToWorkspace5:
                return command_controller->try_move_to_workspace(empty_scope, 5, true);
            case DefaultKeyCommand::MoveToWorkspace6:
                return command_controller->try_move_to_workspace(empty_scope, 6, true);
            case DefaultKeyCommand::MoveToWorkspace7:
                return command_controller->try_move_to_workspace(empty_scope, 7, true);
            case DefaultKeyCommand::MoveToWorkspace8:
                return command_controller->try_move_to_workspace(empty_scope, 8, true);
            case DefaultKeyCommand::MoveToWorkspace9:
                return command_controller->try_move_to_workspace(empty_scope, 9, true);
            case DefaultKeyCommand::MoveToWorkspace0:
                return command_controller->try_move_to_workspace(empty_scope, 0, true);
            case DefaultKeyCommand::ToggleFloating:
                return command_controller->toggle_floating({});
            case DefaultKeyCommand::TogglePinnedToWorkspace:
                return command_controller->toggle_pinned_to_workspace({});
            case DefaultKeyCommand::ToggleTabbing:
                return command_controller->toggle_tabbing({});
            case DefaultKeyCommand::ToggleStacking:
                return command_controller->toggle_stacking({});
            case DefaultKeyCommand::MagnifierOn:
                return magnifier->enable();
            case DefaultKeyCommand::MagnifierOff:
                return magnifier->disable();
            case DefaultKeyCommand::MagnifierIncreaseSize:
            {
                magnifier->set_size(
                    magnifier->get_width() + config->magnifier().size_increment,
                    magnifier->get_height() + config->magnifier().size_increment);
                return true;
            }
            case DefaultKeyCommand::MagnifierDecreaseSize:
            {
                magnifier->set_size(
                    std::max(magnifier->get_width() - config->magnifier().size_increment, 100),
                    std::max(magnifier->get_height() - config->magnifier().size_increment, 100));
                return true;
            }
            case DefaultKeyCommand::MagnifierIncreaseScale:
            {
                magnifier->set_scale(magnifier->get_scale() + config->magnifier().scale_increment);
                return true;
            }
            case DefaultKeyCommand::MagnifierDecreaseScale:
            {
                magnifier->set_scale(std::max(magnifier->get_scale() - config->magnifier().scale_increment, 1.f));
                return true;
            }
            case DefaultKeyCommand::ReloadConfig:
                return command_controller->reload_config();
            default:
                mir::log_error("Unknown key_command: %d", std::to_underlying(key_command));
                break;
            }
            return false;
        }();

        if (handled)
        {
            BindingEvent const binding_event(
                BINDING_MODE_STRINGS[static_cast<size_t>(state->mode())],
                default_key_command_strings[static_cast<size_t>(key_command)],
                modifiers,
                keysym,
                BindingEventType::keyboard);
            binding_event_listener_->on_binding_event(binding_event);
        }

        return handled;
    }))
    {
        return true;
    }

    return false;
}

bool Policy::handle_pointer_event(MirPointerEvent const* event)
{
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    auto const action = miral::toolkit::mir_pointer_event_action(event);
    auto const modifiers = miral::toolkit::mir_pointer_event_modifiers(event) & MODIFIER_MASK;
    auto const buttons = mir_pointer_event_buttons(event);
    state->cursor_position = { x, y };

    if (auto const scene_override = state->scene_override_manager()->try_resolve())
    {
        carousel_controller->break_tap();
        scene_override->handle_pointer_event(event);
        return true;
    }

    if (action == mir_pointer_action_button_down)
        carousel_controller->break_tap();

    // Select the output first
    auto const focused = output_manager->focused();
    for (auto const& output : output_manager->outputs())
    {
        if (output->point_is_in_output(static_cast<int>(x), static_cast<int>(y)))
        {
            if (focused != output)
            {
                if (focused)
                    output_manager->unfocus(focused->id());
                output_manager->focus(output->id());
                if (auto const active = output->active())
                {
                    mir::log_info("Policy::handle_pointer_event: focusing active workspace: %d", active->id());
                    workspace_manager->request_focus(active->id());
                }
            }
            break;
        }
    }

    if (plugin_manager->handle_pointer_event(*event))
        return true;

    if (resize_service->handle_pointer_event(x, y, action))
        return true;

    if (move_service->handle_pointer_event(*state, x, y, action, modifiers, buttons))
        return true;

    if (drag_and_drop_service->handle_pointer_event(*state, x, y, action, modifiers, buttons))
        return true;

    if (output_manager->focused() && state->mode() != WindowManagerMode::resizing)
    {
        // Get Container intersection. Depending on the state, do something with that Container
        switch (state->mode())
        {
        case WindowManagerMode::normal:
        {
            auto const intersected = output_manager->focused()->intersect(x, y);
            if (intersected)
            {
                if (auto const window = intersected->window().value())
                {
                    if (state->focused_container() != intersected && (config->cursor().focus_mode == CursorFocusMode::Hover || action == mir_pointer_action_button_down))
                        window_controller->select_active_window(window);
                }
            }

            break;
        }
        default:
            break;
        }
    }

    return plugin_manager->handle_pointer_event(*event);
}

auto Policy::place_new_window(
    const miral::ApplicationInfo& app_info,
    const miral::WindowSpecification& requested_specification) -> miral::WindowSpecification
{
    if (!output_manager->focused())
    {
        mir::log_warning("place_new_window: no output available");
        return requested_specification;
    }

    // Place the incoming window according to the following criteria:
    // 1. If it is handled by a plugin, then pass it along to the plugin.
    // 2. If it belongs to a shell application, delegate placement to it
    // 3. If it meets the criteria of a shell component, call it one
    // 4. If it is a regular window, allocate it as such on the current workspace
    AllocationHint hint;
    auto new_spec = requested_specification;

    // All windows in Miracle are NOT server side decorated.
    new_spec.server_side_decorated() = false;

    auto const plugin_placement = plugin_manager->place_new_window(app_info, requested_specification, state->peak_next_container_id());
    if (plugin_placement && plugin_placement->strategy == miracle_window_management_strategy_freestyle)
    {
        hint.container_type = AllocationType::plugin;
        hint.workspace = plugin_placement->freestyle.workspace;
        hint.plugin_handle = plugin_placement->freestyle.handle;
        hint.transform = plugin_placement->freestyle.transform;
        hint.alpha = plugin_placement->freestyle.alpha;
        hint.resizable = plugin_placement->freestyle.resizable;
        hint.movable = plugin_placement->freestyle.movable;
        new_spec.top_left() = plugin_placement->freestyle.rectangle.top_left;
        new_spec.size() = plugin_placement->freestyle.rectangle.size;
        new_spec.depth_layer() = plugin_placement->freestyle.layer;
    }
    else if (shell_application_manager->is_registered(app_info.application()))
    {
        if (auto const delegate = shell_application_manager->delegate(app_info.application()))
            delegate->place_window(new_spec);
        hint.container_type = AllocationType::shell;
    }
    else
    {
        auto const has_exclusive_rect = requested_specification.exclusive_rect();
        auto const is_attached = requested_specification.attached_edges();
        if (has_exclusive_rect || is_attached || requested_specification.state() == mir_window_state_attached)
            hint.container_type = AllocationType::shell;
        else
        {
            auto const t = requested_specification.type();
            auto const has_parent = !requested_specification.parent().value_or(std::weak_ptr<mir::scene::Surface>()).expired();
            bool const is_normal_or_freestyle = t == mir_window_type_normal || t == mir_window_type_freestyle;
            bool const wants_fullscreen = requested_specification.state() == mir_window_state_fullscreen;

            // Windows with a parent (popups, dialogs) are freestyle. Normal/freestyle windows without
            // a parent go into the grid, unless they want to be fullscreen by default. Everything else
            // is freestyle.
            if (has_parent)
                hint.container_type = AllocationType::freestyle;
            else if (is_normal_or_freestyle && !wants_fullscreen)
                hint.container_type = AllocationType::grid;
            else
                hint.container_type = AllocationType::freestyle;
        }

        if (hint.container_type != AllocationType::shell && hint.container_type != AllocationType::freestyle)
        {
            // We respect the desired output of the window is one is set.
            std::shared_ptr<AbstractOutput> output;
            if (new_spec.output_id())
            {
                output = output_manager->from(new_spec.output_id().value());
                if (!output)
                    output = output_manager->focused();
            }
            else
                output = output_manager->focused();

            auto parent = output->active()->get_layout_container();
            std::optional<size_t> index;

            // If the plugin placement is tiled, then we're going to try and either:
            // 1. Transform the selected leaf into a parent and place it
            // 2. Place the new window in the selected parent.
            if (plugin_placement && plugin_placement->strategy == miracle_window_management_strategy_tiled)
            {
                if (auto const leaf_container = dynamic_cast<LeafContainer*>(plugin_placement->tiled.container))
                {
                    if (plugin_placement->tiled.scheme != LayoutScheme::none && leaf_container->set_layout(plugin_placement->tiled.scheme))
                    {
                        parent = leaf_container->get_parent().lock().get();
                        index = plugin_placement->tiled.index;
                    }
                    else
                        mir::log_error("Tiled placement referred to a child container but lacked a layout scheme.");
                }
                else
                {
                    parent = dynamic_cast<ParentContainer*>(plugin_placement->tiled.container);
                    index = plugin_placement->tiled.index;
                }
            }

            new_spec = parent->place_new_window(new_spec, index);
            hint.parent = parent;
        }
    }

    // If we're placing a shell window, try and set the output to the focused output.
    // Only do this if the window does not already want to be placed on a specific output.
    if (hint.container_type == AllocationType::shell && !new_spec.output_id())
    {
        if (auto const focused = output_manager->focused())
            new_spec.output_id() = focused->id();
    }

    pending_allocation = hint;
    return new_spec;
}

void Policy::advise_new_window(miral::WindowInfo const& window_info)
{
    if (!output_manager->focused())
    {
        mir::log_error("Policy::advise_new_window: no focused output");
        return;
    }

    auto const container = command_controller->create_container(window_info, pending_allocation);
    (*window_id_map_)[container->id()] = window_info.window();
    pending_allocation.container_type = AllocationType::none;

    if (auto const scene_override = state->scene_override_manager()->try_resolve())
    {
        if (is_carousel_window(container, *window_controller))
            scene_override->handle_window_added(window_info.window());
    }
}

void Policy::advise_new_app(miral::ApplicationInfo& app_info)
{
    (*application_id_map_)[++next_application_id_] = app_info.application();
}

void Policy::advise_delete_app(miral::ApplicationInfo const& app_info)
{
    miral::Application const app = app_info.application();
    for (auto it = application_id_map_->begin(); it != application_id_map_->end(); ++it)
    {
        if (it->second == app)
        {
            application_id_map_->erase(it);
            break;
        }
    }
}

void Policy::handle_window_ready(miral::WindowInfo& window_info)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_window_ready: container is not provided");
        return;
    }

    container->on_open();
    container->handle_ready();
    ipc_command_executor->apply_startup_commands_to(container);
}

mir::geometry::Rectangle
Policy::confirm_placement_on_display(
    miral::WindowInfo const& window_info,
    MirWindowState new_state,
    mir::geometry::Rectangle const& new_placement)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_warning("confirm_placement_on_display: window lacks container");
        return new_placement;
    }

    return container->confirm_placement(new_state, new_placement);
}

void Policy::advise_focus_gained(const miral::WindowInfo& window_info)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("Policy::advise_focus_gained: container is not provided");
        return;
    }

    auto const workspace = container->get_workspace();
    // If the container has a null workspace, it is always selectable. Otherwise
    // it needs to be on the active workspace.
    if (output_manager->focused() && workspace != nullptr && workspace != output_manager->focused()->active())
    {
        // TODO: In this scenario, we may want to navigate to the focused workspace.
        //  This was removed because it breaks workspace animations.
        mir::log_warning("Policy::advise_focus_gained: not selecting a container on an inactive workspace");
        return;
    }

    state->focus_container(container);
    container->on_focus_gained();
    if (workspace)
        workspace->advise_focus_gained(container);
    window_observer_registrar->advise_window_focused(*container);

    // Warning: This must be enqueued because the plugin itself could have
    // triggered the focus, leading to a reentry on the plugin.
    main_loop_->enqueue(this, [window_info, plugin_manager = plugin_manager]
    {
        try
        {
            plugin_manager->window_focused(window_info);
        }
        catch (std::out_of_range const&)
        {
        }
    });
}

void Policy::advise_focus_lost(const miral::WindowInfo& window_info)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_focus_lost: container is not provided");
        return;
    }

    if (state->mode() == WindowManagerMode::dragging)
        drag_and_drop_service->stop_drag(*state);

    state->unfocus_container(container);
    container->on_focus_lost();

    // Warning: This must be enqueued because the plugin itself could have
    // triggered the focus, leading to a reentry on the plugin.
    main_loop_->enqueue(this, [window_info, plugin_manager = plugin_manager]
    {
        try
        {
            plugin_manager->window_unfocused(window_info);
        }
        catch (std::out_of_range const&)
        {
        }
    });
}

void Policy::advise_delete_window(const miral::WindowInfo& window_info)
{
    if (auto const scene_override = state->scene_override_manager()->try_resolve())
        scene_override->handle_window_closed(window_info.window());

    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("delete_container: container is not provided");
        return;
    }

    dying_surface_manager->animate_dying_surface(container);

    // Important: We advise closed before the window has been removed so that it
    // still has valid references inside of it which consumers can use (e.g.
    // a valid parent container)
    window_observer_registrar->advise_closed(*container);
    if (auto const output = container->get_output())
        output->delete_container(container);
    else
        scratchpad_->remove(container);

    animator->remove_by_animation_handle(container->animation_handle());
    if (container == state->focused_container())
        state->unfocus_container(container);

    state->remove(container);
    container->unregister_interest(self.get());
    plugin_manager->window_deleted(window_info);
    for (auto it = window_id_map_->begin(); it != window_id_map_->end(); ++it)
    {
        if (it->second == window_info.window())
        {
            window_id_map_->erase(it);
            break;
        }
    }
}

void Policy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_move_to: container is not provided: %s", window_info.application_id().c_str());
        return;
    }

    container->on_move_to(top_left);
}

void Policy::advise_resize(miral::WindowInfo const& window_info, geom::Size const& new_size)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_move_to: container is not provided: %s", window_info.application_id().c_str());
        return;
    }

    container->on_resize(new_size);
}

void Policy::advise_output_create(miral::Output const& output)
{
    if (auto const scene_override = state->scene_override_manager()->try_resolve())
        scene_override->handle_output_changed();

    mir::log_info("Policy::advise_output_create: %s", output.name().c_str());
    output_manager->create(output.name(), output.id(), output.extents(), *workspace_manager);
    output_listener->output_created(output);
}

void Policy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    if (auto const scene_override = state->scene_override_manager()->try_resolve())
        scene_override->handle_output_changed();

    output_manager->update(updated.id(), updated.extents());
    output_listener->output_updated(updated, original);
}

void Policy::advise_output_delete(miral::Output const& output)
{
    if (auto const scene_override = state->scene_override_manager()->try_resolve())
        scene_override->handle_output_changed();

    output_manager->remove(output.id(), *workspace_manager);
    output_listener->output_deleted(output);
}

void Policy::handle_modify_window(
    miral::WindowInfo& window_info,
    const miral::WindowSpecification& modifications)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_modify_window: container is not provided");
        return;
    }

    // A container is "hidden" when it is genuinely not being rendered: either its
    // workspace is not the active workspace on the workspace's OWN output (note: a
    // workspace active on a non-focused output is still rendered), or it is a hidden
    // scratchpad window. When hidden, the container defers any requested state change
    // until it is shown again; all other modifications are always applied.
    bool hidden = false;
    if (auto const workspace = container->get_workspace())
    {
        auto const output = workspace->get_output();
        hidden = output && output->active().get() != workspace.get();
    }
    else if (scratchpad_->contains(container) && !scratchpad_->is_showing(container))
    {
        hidden = true;
    }

    container->handle_modify(modifications, hidden);
}

#ifdef MIR_VERSION_2_29_OR_GREATER
void Policy::handle_activate_window(miral::WindowInfo& window_info)
#else
void Policy::handle_raise_window(miral::WindowInfo& window_info)
#endif
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_activate_window: container is not provided");
        return;
    }

    container->handle_raise();
}

bool Policy::handle_touch_event(const MirTouchEvent* event)
{
    // Swallow touch input while a scene override is active, consistent with
    // keyboard and pointer handling.
    return state->scene_override_manager()->try_resolve() != nullptr;
}

void Policy::handle_request_move(miral::WindowInfo& window_info, const MirInputEvent* input_event)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("Policy::handle_request_move: window lacks container");
        return;
    }

    auto const pointer_event = mir_input_event_get_pointer_event(input_event);
    auto const x = miral::toolkit::mir_pointer_event_axis_value(pointer_event, MirPointerAxis::mir_pointer_axis_x);
    auto const y = miral::toolkit::mir_pointer_event_axis_value(pointer_event, MirPointerAxis::mir_pointer_axis_y);
    auto const action = miral::toolkit::mir_pointer_event_action(pointer_event);
    auto const buttons = miral::toolkit::mir_pointer_event_buttons(pointer_event);
    move_service->handle_pointer_event(
        *state, x, y, action, config->process_modifier(config->move_modifier()), buttons);
}

void Policy::handle_request_resize(
    miral::WindowInfo& window_info,
    const MirInputEvent* input_event,
    MirResizeEdge edge)
{
    auto const container = window_controller->get_window_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_request_resize: window lacks container");
        return;
    }

    auto const pointer_event = mir_input_event_get_pointer_event(input_event);
    auto const action = miral::toolkit::mir_pointer_event_action(pointer_event);
    auto const x = miral::toolkit::mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x);
    auto const y = miral::toolkit::mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y);
    resize_service->handle_request_resize(container, action, edge, x, y);
}

mir::geometry::Rectangle Policy::confirm_inherited_move(
    const miral::WindowInfo& window_info,
    mir::geometry::Displacement movement)
{
    return { window_info.window().top_left() + movement, window_info.window().size() };
}

void Policy::advise_application_zone_create(miral::Zone const& application_zone)
{
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_create(application_zone);
    }
}

void Policy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_update(updated, original);
    }
}

void Policy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_delete(application_zone);
    }
}

void Policy::advise_end()
{
    if (is_starting_ && output_manager->focused())
    {
        is_starting_ = false;
        config->reload();
    }
}
