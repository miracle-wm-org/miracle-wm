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

#include "window_metadata.h"
#include "window_tools_accessor.h"
#define MIR_LOG_COMPONENT "miracle"

#include "miracle_config.h"
#include "policy.h"
#include "window_helpers.h"
#include "workspace_manager.h"

#include <iostream>
#include <limits>
#include <mir/geometry/rectangle.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir_toolkit/events/enums.h>
#include <miral/application_info.h>
#include <miral/runner.h>
#include <miral/toolkit_event.h>
#include <miral/window_specification.h>
#include <miral/zone.h>

using namespace miracle;

namespace
{
const int MODIFIER_MASK = mir_input_event_modifier_alt | mir_input_event_modifier_shift | mir_input_event_modifier_sym | mir_input_event_modifier_ctrl | mir_input_event_modifier_meta;
}

Policy::Policy(
    miral::WindowManagerTools const& tools,
    AutoRestartingLauncher& external_client_launcher,
    miral::MirRunner& runner,
    std::shared_ptr<MiracleConfig> const& config,
    SurfaceTracker& surface_tracker,
    mir::Server const& server) :
    window_manager_tools { tools },
    floating_window_manager(tools, config->get_input_event_modifier()),
    external_client_launcher { external_client_launcher },
    runner { runner },
    config { config },
    workspace_manager { WorkspaceManager(
        tools,
        workspace_observer_registrar,
        [&]()
{ return get_active_output(); }) },
    i3_command_executor(*this, workspace_manager, tools, external_client_launcher),
    surface_tracker { surface_tracker },
    ipc { std::make_shared<Ipc>(runner, workspace_manager, *this, server.the_main_loop(), i3_command_executor, config) },
    animator(server.the_main_loop(), config),
    node_interface(tools, animator, state)
{
    animator.start();
    workspace_observer_registrar.register_interest(ipc);
    mode_observer_registrar.register_interest(ipc);
    WindowToolsAccessor::get_instance().set_tools(tools);
}

Policy::~Policy()
{
    workspace_observer_registrar.unregister_interest(ipc.get());
    mode_observer_registrar.unregister_interest(ipc.get());
}

bool Policy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

    auto custom_key_command = config->matches_custom_key_command(action, scan_code, modifiers);
    if (custom_key_command != nullptr)
    {
        external_client_launcher.launch({ custom_key_command->command });
        return true;
    }

    return config->matches_key_command(action, scan_code, modifiers, [&](DefaultKeyCommand key_command)
    {
        if (key_command == DefaultKeyCommand::MAX)
            return false;

        switch (key_command)
        {
        case Terminal:
        {
            auto terminal_command = config->get_terminal_command();
            if (terminal_command)
                external_client_launcher.launch({ terminal_command.value() });
            return true;
        }
        case RequestVertical:
            return try_request_vertical();
        case RequestHorizontal:
            return try_request_horizontal();
        case ToggleResize:
            try_toggle_resize_mode();
            return true;
        case ResizeUp:
            return try_resize(Direction::up);
        case ResizeDown:
            return try_resize(Direction::down);
        case ResizeLeft:
            return try_resize(Direction::left);
        case ResizeRight:
            return try_resize(Direction::right);
        case MoveUp:
            return try_move(Direction::up);
        case MoveDown:
            return try_move(Direction::down);
        case MoveLeft:
            return try_move(Direction::left);
        case MoveRight:
            return try_move(Direction::right);
        case SelectUp:
            return try_select(Direction::up);
        case SelectDown:
            return try_select(Direction::down);
        case SelectLeft:
            return try_select(Direction::left);
        case SelectRight:
            return try_select(Direction::right);
        case QuitActiveWindow:
            return try_close_window();
        case QuitCompositor:
            return quit();
        case Fullscreen:
            return try_toggle_fullscreen();
        case SelectWorkspace1:
            return select_workspace(1);
        case SelectWorkspace2:
            return select_workspace(2);
        case SelectWorkspace3:
            return select_workspace(3);
        case SelectWorkspace4:
            return select_workspace(4);
        case SelectWorkspace5:
            return select_workspace(5);
        case SelectWorkspace6:
            return select_workspace(6);
        case SelectWorkspace7:
            return select_workspace(7);
        case SelectWorkspace8:
            return select_workspace(8);
        case SelectWorkspace9:
            return select_workspace(9);
        case SelectWorkspace0:
            return select_workspace(0);
        case MoveToWorkspace1:
            return move_active_to_workspace(1);
        case MoveToWorkspace2:
            return move_active_to_workspace(2);
        case MoveToWorkspace3:
            return move_active_to_workspace(3);
        case MoveToWorkspace4:
            return move_active_to_workspace(4);
        case MoveToWorkspace5:
            return move_active_to_workspace(5);
        case MoveToWorkspace6:
            return move_active_to_workspace(6);
        case MoveToWorkspace7:
            return move_active_to_workspace(7);
        case MoveToWorkspace8:
            return move_active_to_workspace(8);
        case MoveToWorkspace9:
            return move_active_to_workspace(9);
        case MoveToWorkspace0:
            return move_active_to_workspace(0);
        case ToggleFloating:
            return toggle_floating();
        case TogglePinnedToWorkspace:
            return toggle_pinned_to_workspace();
        default:
            std::cerr << "Unknown key_command: " << key_command << std::endl;
            break;
        }
        return false;
    });
}

bool Policy::handle_pointer_event(MirPointerEvent const* event)
{
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    state.cursor_position = { x, y };

    for (auto const& output : output_list)
    {
        if (output->point_is_in_output(static_cast<int>(x), static_cast<int>(y)))
        {
            if (active_output != output)
            {
                if (active_output)
                    active_output->set_is_active(false);
                active_output = output;
                active_output->set_is_active(true);
                workspace_manager.request_focus(output->get_active_workspace_num());
            }

            if (output->get_active_workspace_num() >= 0 && state.mode != WindowManagerMode::resizing)
            {
                active_output->select_window_from_point(static_cast<int>(x), static_cast<int>(y));
            }
            break;
        }
    }

    if (active_output)
        return active_output->handle_pointer_event(event);

    return false;
}

auto Policy::place_new_window(
    const miral::ApplicationInfo& app_info,
    const miral::WindowSpecification& requested_specification) -> miral::WindowSpecification
{
    if (!active_output)
    {
        mir::log_warning("place_new_window: no output available");
        return requested_specification;
    }

    auto new_spec = requested_specification;
    pending_output = active_output;
    pending_type = active_output->allocate_position(new_spec);
    return new_spec;
}

void Policy::advise_new_window(miral::WindowInfo const& window_info)
{
    auto shared_output = pending_output.lock();
    if (!shared_output)
    {
        mir::log_warning("advise_new_window: output unavailable");
        auto window = window_info.window();
        if (!output_list.empty())
        {
            // Our output is gone! Let's try to add it to a different output
            output_list.front()->add_immediately(window);
        }
        else
        {
            // We have no output! Let's add it to a list of orphans. Such
            // windows are considered to be in the "other" category until
            // we have more data on them.
            orphaned_window_list.push_back(window);
            surface_tracker.add(window);
            auto metadata = std::make_shared<WindowMetadata>(WindowType::other, window_info.window());
            miral::WindowSpecification spec;
            spec.userdata() = metadata;
            window_manager_tools.modify_window(window, spec);
        }

        return;
    }

    auto metadata = shared_output->advise_new_window(window_info, pending_type);

    // Associate to an animation handle
    metadata->set_animation_handle(animator.register_animateable());
    node_interface.open(window_info.window());

    pending_type = WindowType::none;
    pending_output.reset();

    surface_tracker.add(window_info.window());
}

void Policy::handle_window_ready(miral::WindowInfo& window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("handle_window_ready: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->handle_window_ready(window_info, metadata);
}

void Policy::advise_focus_gained(const miral::WindowInfo& window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("advise_focus_gained: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->advise_focus_gained(metadata);
    else
        window_manager_tools.raise_tree(window_info.window());
}

void Policy::advise_focus_lost(const miral::WindowInfo& window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("advise_focus_lost: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->advise_focus_lost(metadata);
}

void Policy::advise_delete_window(const miral::WindowInfo& window_info)
{
    for (auto it = orphaned_window_list.begin(); it != orphaned_window_list.end(); it++)
    {
        if (*it == window_info.window())
        {
            orphaned_window_list.erase(it);
            surface_tracker.remove(window_info.window());
            return;
        }
    }

    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("advise_delete_window: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->advise_delete_window(metadata);

    surface_tracker.remove(window_info.window());
}

void Policy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("advise_move_to: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->advise_move_to(metadata, top_left);
}

void Policy::advise_output_create(miral::Output const& output)
{
    auto new_tree = std::make_shared<OutputContent>(
        output, workspace_manager, output.extents(), window_manager_tools,
        floating_window_manager, config, node_interface, animator);
    workspace_manager.request_first_available_workspace(new_tree);
    output_list.push_back(new_tree);
    if (active_output == nullptr)
        active_output = new_tree;

    // Let's rehome some orphan windows if we need to
    if (!orphaned_window_list.empty())
    {
        for (auto& window : orphaned_window_list)
        {
            active_output->add_immediately(window);
        }
        orphaned_window_list.clear();
    }
}

void Policy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    for (auto& output : output_list)
    {
        if (output->get_output().is_same_output(original))
        {
            output->update_area(updated.extents());
            break;
        }
    }
}

void Policy::advise_output_delete(miral::Output const& output)
{
    for (auto it = output_list.begin(); it != output_list.end();)
    {
        auto other_output = *it;
        if (other_output->get_output().is_same_output(output))
        {
            output_list.erase(it);
            if (output_list.empty())
            {
                // All nodes should become orphaned
                for (auto& window : other_output->collect_all_windows())
                {
                    orphaned_window_list.push_back(window);
                    WindowSpecification spec;
                    spec.userdata() = nullptr;
                    window_manager_tools.modify_window(window, spec);
                }

                active_output = nullptr;
            }
            else
            {
                active_output = output_list.front();
                for (auto& window : other_output->collect_all_windows())
                {
                    active_output->add_immediately(window);
                }
            }
            break;
        }
    }
}

void Policy::handle_modify_window(
    miral::WindowInfo& window_info,
    const miral::WindowSpecification& modifications)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("handle_modify_window: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->handle_modify_window(metadata, modifications);
    else
        window_manager_tools.modify_window(metadata->get_window(), modifications);
}

void Policy::handle_raise_window(miral::WindowInfo& window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("handle_raise_window: metadata is not provided");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->handle_raise_window(metadata);
}

mir::geometry::Rectangle
Policy::confirm_placement_on_display(
    const miral::WindowInfo& window_info,
    MirWindowState new_state,
    const mir::geometry::Rectangle& new_placement)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_warning("confirm_placement_on_display: window lacks metadata");
        return new_placement;
    }

    mir::geometry::Rectangle modified_placement = metadata->get_output()
        ? metadata->get_output()->confirm_placement_on_display(metadata, new_state, new_placement)
        : new_placement;
    return modified_placement;
}

bool Policy::handle_touch_event(const MirTouchEvent* event)
{
    return false;
}

void Policy::handle_request_move(miral::WindowInfo& window_info, const MirInputEvent* input_event)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("handle_request_move: window lacks metadata");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->handle_request_move(metadata, input_event);
}

void Policy::handle_request_resize(
    miral::WindowInfo& window_info,
    const MirInputEvent* input_event,
    MirResizeEdge edge)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("handle_request_resize: window lacks metadata");
        return;
    }

    if (metadata->get_output())
        metadata->get_output()->handle_request_resize(metadata, input_event, edge);
}

mir::geometry::Rectangle Policy::confirm_inherited_move(
    const miral::WindowInfo& window_info,
    mir::geometry::Displacement movement)
{
    return { window_info.window().top_left() + movement, window_info.window().size() };
}

void Policy::advise_application_zone_create(miral::Zone const& application_zone)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_create(application_zone);
    }
}

void Policy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_update(updated, original);
    }
}

void Policy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_delete(application_zone);
    }
}

void Policy::try_toggle_resize_mode()
{
    if (!active_output)
    {
        state.mode = WindowManagerMode::normal;
        return;
    }

    auto const& window = active_output->get_active_window();
    if (!window)
    {
        state.mode = WindowManagerMode::normal;
        return;
    }

    auto metadata = window_helpers::get_metadata(window, window_manager_tools);
    if (!metadata)
    {
        state.mode = WindowManagerMode::normal;
        return;
    }

    if (metadata->get_type() != WindowType::tiled)
    {
        state.mode = WindowManagerMode::normal;
        return;
    }

    if (state.mode == WindowManagerMode::resizing)
        state.mode = WindowManagerMode::normal;
    else
        state.mode = WindowManagerMode::resizing;

    mode_observer_registrar.advise_changed(state.mode);
}

bool Policy::try_request_vertical()
{
    if (state.mode != WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    active_output->request_vertical();
    return true;
}

bool Policy::try_request_horizontal()
{
    if (state.mode != WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    active_output->request_horizontal();
    return true;
}

bool Policy::try_resize(miracle::Direction direction)
{
    if (state.mode != WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    return active_output->resize_active_window(direction);
}

bool Policy::try_move(miracle::Direction direction)
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    return active_output->move_active_window(direction);
}

bool Policy::try_select(miracle::Direction direction)
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    return active_output->select(direction);
}

bool Policy::try_close_window()
{
    if (!active_output)
        return false;

    active_output->close_active_window();
    return true;
}

bool Policy::quit()
{
    runner.stop();
    return true;
}

bool Policy::try_toggle_fullscreen()
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    active_output->toggle_fullscreen();
    return true;
}

bool Policy::select_workspace(int number)
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    workspace_manager.request_workspace(active_output, number);
    return true;
}

bool Policy::move_active_to_workspace(int number)
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    workspace_manager.move_active_to_workspace(active_output, number);
    return true;
}

bool Policy::toggle_floating()
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    active_output->request_toggle_active_float();
    return true;
}

bool Policy::toggle_pinned_to_workspace()
{
    if (state.mode == WindowManagerMode::resizing)
        return false;

    if (!active_output)
        return false;

    active_output->toggle_pinned_to_workspace();
    return true;
}
