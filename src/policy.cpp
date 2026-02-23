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

#define MIR_LOG_COMPONENT "miracle"

#include "policy.h"
#include "animator_loop.h"
#include "binding_event.h"
#include "config.h"
#include "config_observer.h"
#include "constants.h"
#include "container_group_container.h"
#include "container_listener.h"
#include "dying_surface_manager.h"
#include "feature_flags.h"
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
#include "shell_application_manager.h"
#include "shell_component_container.h"
#include "window_observer.h"
#include "workspace_manager.h"

#include <iostream>
#include <mir/geometry/rectangle.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir_toolkit/events/enums.h>
#include <miral/toolkit_event.h>
#include <miral/window_specification.h>
#include <mutex>

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
    explicit Self(Policy& policy) :
        policy { policy }
    {
    }

    void on_workspace_created(uint32_t) override { }
    void on_workspace_removed(uint32_t) override { }
    void on_workspace_empty(uint32_t) override { }
    void on_workspace_focused(std::optional<uint32_t> old, uint32_t next) override
    {
        if (old)
        {
            auto const& last_workspace = policy.workspace_manager->workspace(old.value());
            if (!last_workspace)
            {
                mir::log_error("Policy::Self::on_focused missing last workspace");
                return;
            }

            auto const& next_workspace = policy.workspace_manager->workspace(next);
            if (!next_workspace)
            {
                mir::log_error("Policy::Self::on_focused missing next workspace");
                return;
            }

            if (last_workspace->get_output() != next_workspace->get_output())
                policy.command_controller->move_cursor_to_output(*next_workspace->get_output());
        }
    }
    void on_workspace_renamed(uint32_t) override { }

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

    void on_config_changed(Config const& config) override
    {
        // Note: We need to grab the lock because this notification comes from
        // a different thread.
        auto const lock = policy.state->lock();
        for (auto const& output : policy.output_manager->outputs())
        {
            for (auto const& workspace : output->get_workspaces())
                workspace->recalculate_area();
        }

        if (!has_loaded_once)
        {
            if (config.magnifier().enabled)
                policy.magnifier->enable();
            else
                policy.magnifier->disable();
        }

        policy.magnifier->set_scale(config.magnifier().scale);
        policy.magnifier->set_size(config.magnifier().width, config.magnifier().height);

        policy.plugin_manager->unload_all();
        for (auto const& plugin : config.get_plugins())
            policy.plugin_manager->load_wasm_module(plugin.path);

        has_loaded_once = true;
    }

    Policy& policy;
    bool has_loaded_once = false;
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
    miral::Magnifier const& magnifier) :
    tools { tools },
    config { config },
    state { state },
    output_listener { output_listener },
    config_observer_registrar { config_observer_registrar },
    animator(std::make_shared<Animator>()),
    plugin_manager(std::make_shared<PluginManager>()),
    window_controller(std::make_shared<WindowManagerToolsWindowController>(
        tools, animator, plugin_manager, server.the_main_loop(), state, config)),
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
            server.the_main_loop(),
            plugin_manager))),
    workspace_manager(std::make_shared<WorkspaceManager>(workspace_observer_registrar, config, output_manager)),
    self(std::make_shared<Self>(*this)),
    scratchpad_(std::make_shared<Scratchpad>(window_controller, output_manager)),
    command_controller(std::make_shared<CommandController>(
        config, state, window_controller,
        workspace_manager, mode_observer_registrar,
        std::make_unique<MirRunnerCommandControllerInterface>(server.the_main_loop()), scratchpad_, output_manager)),
    drag_and_drop_service(std::make_unique<DragAndDropService>(command_controller, config, output_manager)),
    move_service(std::make_unique<MoveService>(command_controller, config, output_manager)),
    resize_service(std::make_unique<ResizeService>(command_controller, config, state, output_manager)),
    ipc_command_executor(std::make_shared<IpcCommandExecutor>(command_controller, launcher)),
    ipc_connection_manager(std::make_shared<IpcConnectionManager>(
        server.the_main_loop(),
        command_controller,
        ipc_command_executor,
        config)),
    animator_loop(std::make_unique<ThreadedAnimatorLoop>(animator)),
    main_loop_(server.the_main_loop()),
    dying_surface_manager(std::make_unique<DyingSurfaceManager>(
        server.the_surface_stack(),
        state,
        config,
        animator,
        plugin_manager)),
    window_observer_registrar(std::make_unique<WindowObserverRegistrar>()),
    magnifier(std::make_unique<MagnifierWrapper>(magnifier))
{
    plugin_manager->initialize(std::make_unique<PluginBridge>(output_manager, window_controller, workspace_manager, state));
    workspace_observer_registrar->register_interest(ipc_connection_manager);
    workspace_observer_registrar->register_interest(self);
    mode_observer_registrar->register_interest(ipc_connection_manager);
    window_observer_registrar->register_interest(ipc_connection_manager);
    config_observer_registrar->register_interest(ipc_connection_manager);
    output_listener->register_listener(ipc_connection_manager);
    config_observer_registrar->register_interest(self);
    animator_loop->start();
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
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;
    auto const keysym = miral::toolkit::mir_keyboard_event_keysym(event);

    if (auto const custom_key_command = config->matches_custom_key_command(action, scan_code, modifiers))
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

    if (config->matches_key_command(action, scan_code, modifiers, [&](DefaultKeyCommand key_command)
    {
        if (key_command == DefaultKeyCommand::MAX)
            return false;

        BindingEvent const binding_event(
            BINDING_MODE_STRINGS[static_cast<size_t>(state->mode())],
            default_key_command_strings[static_cast<size_t>(key_command)],
            modifiers,
            keysym,
            BindingEventType::keyboard);
        ipc_connection_manager->on_binding_event(binding_event);

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
        default:
            mir::log_error("Unknown key_command: %d", std::to_underlying(key_command));
            break;
        }
        return false;
    }))
    {
        return true;
    }

    return plugin_manager->handle_keyboard_event(*event);
}

bool Policy::handle_pointer_event(MirPointerEvent const* event)
{
    auto const lock = state->lock();
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    auto const action = miral::toolkit::mir_pointer_event_action(event);
    auto const modifiers = miral::toolkit::mir_pointer_event_modifiers(event) & MODIFIER_MASK;
    auto const buttons = mir_pointer_event_buttons(event);
    state->cursor_position = { x, y };

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

    if (resize_service->handle_pointer_event(x, y, action))
        return true;

    if (move_service->handle_pointer_event(*state, x, y, action, modifiers, buttons))
        return true;

    if (drag_and_drop_service->handle_pointer_event(*state, x, y, action, modifiers, buttons))
        return true;

    if (output_manager->focused() && state->mode() != WindowManagerMode::resizing)
    {
        if (feature::multi_select && action == mir_pointer_action_button_down)
        {
            if (modifiers == config->get_primary_modifier())
            {
                // We clicked while holding the modifier, so we're probably in the middle of a multi-selection.
                if (state->mode() != WindowManagerMode::selecting)
                {
                    command_controller->set_mode(WindowManagerMode::selecting);
                    group_selection = std::make_shared<ContainerGroupContainer>(state);
                    state->add(group_selection);
                }
            }
            else if (state->mode() == WindowManagerMode::selecting)
            {
                // We clicked while we were in selection mode, so let's stop being in selection mode
                // TODO: Would it be better to check what we clicked in case it's in the group? Then we wouldn't
                //  exit selection mode in this case.
                command_controller->set_mode(WindowManagerMode::normal);
            }
        }

        // Get Container intersection. Depending on the state, do something with that Container
        std::shared_ptr<Container> intersected = output_manager->focused()->intersect(x, y);
        switch (state->mode())
        {
        case WindowManagerMode::normal:
        {
            if (intersected)
            {
                if (auto window = intersected->window().value())
                {
                    if (state->focused_container() != intersected && (config->cursor().focus_mode == CursorFocusMode::Hover || action == mir_pointer_action_button_down))
                        window_controller->select_active_window(window);
                }
            }

            return false;
        }
        case WindowManagerMode::selecting:
        {
            if (intersected && action == mir_pointer_action_button_down)
                group_selection->add(intersected);
            return true;
        }
        default:
            return false;
        }
    }

    return false;
}

auto Policy::place_new_window(
    const miral::ApplicationInfo& app_info,
    const miral::WindowSpecification& requested_specification) -> miral::WindowSpecification
{
    auto const lock = state->lock();
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

    auto const plugin_placement = plugin_manager->place_new_window(app_info, requested_specification);
    if (plugin_placement && plugin_placement->strategy == miracle_window_management_strategy_freestyle)
    {
        hint.container_type = ContainerType::plugin;
        hint.workspace = plugin_placement->freestyle.workspace;
        hint.plugin_handle = plugin_placement->freestyle.handle;
        new_spec.top_left() = plugin_placement->freestyle.rectangle.top_left;
        new_spec.size() = plugin_placement->freestyle.rectangle.size;
        new_spec.depth_layer() = plugin_placement->freestyle.layer;
    }
    else if (shell_application_manager->is_registered(app_info.application()))
    {
        if (auto const delegate = shell_application_manager->delegate(app_info.application()))
            delegate->place_window(new_spec);
        hint.container_type = ContainerType::shell;
    }
    else
    {
        auto const has_exclusive_rect = requested_specification.exclusive_rect().is_set();
        auto const is_attached = requested_specification.attached_edges().is_set();
        auto const wrong_leaf_state = requested_specification.state() == mir_window_state_hidden
            || requested_specification.state() == mir_window_state_attached;

        if (has_exclusive_rect || is_attached || wrong_leaf_state)
            hint.container_type = ContainerType::shell;
        else
        {
            auto const t = requested_specification.type();
            if (t == mir_window_type_normal || t == mir_window_type_freestyle)
                hint.container_type = ContainerType::regular;
            else
                hint.container_type = ContainerType::shell; // This is probably a tooltip or something
        }

        if (hint.container_type != ContainerType::shell)
        {
            auto parent = output_manager->focused()->active()->get_layout_container();
            std::optional<size_t> index;

            // If the plugin placement is tiled, then we're going to try and either:
            // 1. Transform the selected leaf into a parent and place it
            // 2. Place the new window in the selected parent.
            if (plugin_placement && plugin_placement->strategy == miracle_window_management_strategy_tiled)
            {
                if (plugin_placement->tiled.container->is_leaf())
                {
                    auto const leaf_container = dynamic_cast<LeafContainer*>(plugin_placement->tiled.container);
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

            new_spec = parent->place_new_window(requested_specification, index);
            hint.parent = parent;
        }
    }

    pending_allocation = hint;
    return new_spec;
}

void Policy::advise_new_window(miral::WindowInfo const& window_info)
{
    auto const lock = state->lock();
    if (!output_manager->focused())
    {
        mir::log_error("Policy::advise_new_window: no focused output");
        return;
    }

    miral::WindowSpecification spec;
    std::shared_ptr<Container> container;
    switch (pending_allocation.container_type)
    {
    case ContainerType::regular:
    {
        assert(pending_allocation.parent);
        container = pending_allocation.parent->confirm_window(window_info.window());
        spec.min_width() = mir::geometry::Width(0);
        spec.min_height() = mir::geometry::Height(0);
        break;
    }
    case ContainerType::plugin:
    {
        auto const workspace = pending_allocation.workspace
            ? pending_allocation.workspace->shared_from_this()
            : output_manager->focused()->active();
        container = std::make_shared<PluginManagedContainer>(
            pending_allocation.plugin_handle,
            window_info.window(),
            window_controller,
            state);
        workspace->add_other_container(container);
    }
    break;
    case ContainerType::shell:
    default:
        container = std::make_shared<ShellComponentContainer>(window_info.window(), window_controller, shell_application_manager->delegate(window_info.window().application()));
        break;
    }

    spec.userdata() = container;
    window_controller->modify(window_info.window(), spec);

    container->animation_handle(animator->register_animateable());
    container->on_open();
    state->add(container);

    window_observer_registrar->advise_created(*container);
    container->register_interest(self);
    pending_allocation.container_type = ContainerType::none;
}

void Policy::handle_window_ready(miral::WindowInfo& window_info)
{
    auto const lock = state->lock();
    auto const container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_window_ready: container is not provided");
        return;
    }

    container->handle_ready();
    ipc_command_executor->apply_startup_commands_to(container);
}

mir::geometry::Rectangle
Policy::confirm_placement_on_display(
    miral::WindowInfo const& window_info,
    MirWindowState new_state,
    mir::geometry::Rectangle const& new_placement)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_warning("confirm_placement_on_display: window lacks container");
        return new_placement;
    }

    return container->confirm_placement(new_state, new_placement);
}

void Policy::advise_focus_gained(const miral::WindowInfo& window_info)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("Policy::advise_focus_gained: container is not provided");
        return;
    }

    switch (state->mode())
    {
    case WindowManagerMode::selecting:
        group_selection->add(container);
        container->on_focus_gained();
        break;
    default:
    {
        auto const workspace = container->get_workspace();

        // If the container has a null workspace, it is always selectable. Otherwise
        // it needs to be on the active workspace.
        if (output_manager->focused() && workspace != nullptr && workspace != output_manager->focused()->active())
        {
            // TODO: In this scenario, we may want to navigate to the focused workspace.
            //  This was removed because it breaks workspace animations.
            mir::log_warning("Policy::advise_focus_gained: not selecting a container on an inactive workspace");
            break;
        }

        state->focus_container(container);
        container->on_focus_gained();
        if (workspace)
            workspace->advise_focus_gained(container);
        window_observer_registrar->advise_window_focused(*container);
        break;
    }
    }
}

void Policy::advise_focus_lost(const miral::WindowInfo& window_info)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_focus_lost: container is not provided");
        return;
    }

    if (state->mode() == WindowManagerMode::dragging)
    {
        command_controller->set_mode(WindowManagerMode::normal);
        if (state->focused_container())
            state->focused_container()->drag_stop();
    }

    state->unfocus_container(container);
    container->on_focus_lost();
}

void Policy::advise_delete_window(const miral::WindowInfo& window_info)
{
    auto const lock = state->lock();
    auto const container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("delete_container: container is not provided");
        return;
    }

    plugin_manager->window_deleted(window_info);

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

    dying_surface_manager->animate_dying_surface(container);
    container->unregister_interest(self.get());
}

void Policy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_move_to: container is not provided: %s", window_info.application_id().c_str());
        return;
    }

    container->on_move_to(top_left);
}

void Policy::advise_resize(miral::WindowInfo const& window_info, geom::Size const& new_size)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_move_to: container is not provided: %s", window_info.application_id().c_str());
        return;
    }

    container->on_resize(new_size);
}

void Policy::advise_output_create(miral::Output const& output)
{
    mir::log_info("Policy::advise_output_create: %s", output.name().c_str());
    auto const lock = state->lock();
    output_manager->create(output.name(), output.id(), output.extents(), *workspace_manager);
    output_listener->output_created(output);
}

void Policy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    auto const lock = state->lock();
    output_manager->update(updated.id(), updated.extents());
    output_listener->output_updated(updated, original);
}

void Policy::advise_output_delete(miral::Output const& output)
{
    auto const lock = state->lock();
    output_manager->remove(output.id(), *workspace_manager);
    output_listener->output_deleted(output);
}

void Policy::handle_modify_window(
    miral::WindowInfo& window_info,
    const miral::WindowSpecification& modifications)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_modify_window: container is not provided");
        return;
    }

    auto const workspace = container->get_workspace();
    if (workspace)
    {
        auto focused_output = output_manager->focused();
        if (!focused_output)
        {
            mir::log_error("Policy::handle_modify_window: focused_output unavailable");
            return;
        }

        if (workspace != focused_output->active())
            return;
    }
    else if (scratchpad_->contains(container) && !scratchpad_->is_showing(container))
        return;

    container->handle_modify(modifications);
}

void Policy::handle_raise_window(miral::WindowInfo& window_info)
{
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_raise_window: container is not provided");
        return;
    }

    container->handle_raise();
}

bool Policy::handle_touch_event(const MirTouchEvent* event)
{
    return false;
}

void Policy::handle_request_move(miral::WindowInfo& window_info, const MirInputEvent* input_event)
{
    auto const lock = state->lock();
    auto const container = window_controller->get_container(window_info.window());
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
    auto const lock = state->lock();
    auto container = window_controller->get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_request_resize: window lacks container");
        return;
    }

    auto const pointer_event = mir_input_event_get_pointer_event(input_event);
    auto const action = miral::toolkit::mir_pointer_event_action(pointer_event);
    resize_service->handle_request_resize(container, action, edge);
}

mir::geometry::Rectangle Policy::confirm_inherited_move(
    const miral::WindowInfo& window_info,
    mir::geometry::Displacement movement)
{
    return { window_info.window().top_left() + movement, window_info.window().size() };
}

void Policy::advise_application_zone_create(miral::Zone const& application_zone)
{
    auto const lock = state->lock();
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_create(application_zone);
    }
}

void Policy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    auto const lock = state->lock();
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_update(updated, original);
    }
}

void Policy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    auto const lock = state->lock();
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
        for (auto const& app : config->get_startup_apps())
        {
            launcher->launch(app);
        }

        // TODO: This is very weird, but it seems like mouse and keyboard
        //  configuration events will not be piped through until things are
        //  up and running, so I guess we're going to do it here!
        config_observer_registrar->advise_config_changed(*config);
    }
}
