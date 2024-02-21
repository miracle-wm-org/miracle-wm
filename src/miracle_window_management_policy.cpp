#define MIR_LOG_COMPONENT "miracle"

#include "miracle_window_management_policy.h"
#include "window_helpers.h"
#include "miracle_config.h"
#include "workspace_manager.h"

#include <mir_toolkit/events/enums.h>
#include <miral/toolkit_event.h>
#include <miral/application_info.h>
#include <miral/zone.h>
#include <miral/runner.h>
#include <mir/log.h>
#include <iostream>
#include <mir/geometry/rectangle.h>
#include <limits>

using namespace miracle;

namespace
{
const int MODIFIER_MASK =
    mir_input_event_modifier_alt |
    mir_input_event_modifier_shift |
    mir_input_event_modifier_sym |
    mir_input_event_modifier_ctrl |
    mir_input_event_modifier_meta;

// Note: This list was taken from i3: https://github.com/i3/i3/blob/next/i3-sensible-terminal
// We will want this to be configurable in the future.
const std::string POSSIBLE_TERMINALS[] = {
    "x-terminal-emulator",
    "mate-terminal",
    "gnome-terminal",
    "terminator",
    "xfce4-terminal",
    "urxvt", "rxvt",
    "termit",
    "Eterm",
    "aterm",
    "uxterm",
    "xterm",
    "roxterm",
    "termite",
    "lxterminal",
    "terminology",
    "st",
    "qterminal",
    "lilyterm",
    "tilix",
    "terminix",
    "konsole",
    "kitty",
    "guake",
    "tilda",
    "alacritty",
    "hyper",
    "wezterm",
    "rio"
};
}

MiracleWindowManagementPolicy::MiracleWindowManagementPolicy(
    miral::WindowManagerTools const& tools,
    miral::ExternalClientLauncher const& external_client_launcher,
    miral::InternalClientLauncher const& internal_client_launcher,
    miral::MirRunner& runner,
    MiracleConfig const& config)
    : window_manager_tools{tools},
      external_client_launcher{external_client_launcher},
      internal_client_launcher{internal_client_launcher},
      runner{runner},
      config{config},
      workspace_manager{WorkspaceManager(
          tools,
          workspace_observer_registrar,
          [&]() { return get_active_output(); })},
      ipc{std::make_shared<Ipc>(runner, workspace_manager)}
{
    workspace_observer_registrar.register_interest(ipc);
}

bool MiracleWindowManagementPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

    auto custom_key_command = config.matches_custom_key_command(action, scan_code, modifiers);
    if (custom_key_command != nullptr)
    {
        external_client_launcher.launch(custom_key_command->command);
        return true;
    }

    auto key_command = config.matches_key_command(action, scan_code, modifiers);
    if (key_command == DefaultKeyCommand::MAX)
        return false;

    switch (key_command)
    {
        case Terminal:
            for (auto const& terminal : POSSIBLE_TERMINALS)
            {
                if (external_client_launcher.launch({terminal}) > 0)
                    break;
            }
            return true;
        case RequestVertical:
            if(active_output) active_output->get_active_tree().request_vertical();
            return true;
        case RequestHorizontal:
            if(active_output) active_output->get_active_tree().request_horizontal();
            return true;
        case ToggleResize:
            if(active_output) active_output->get_active_tree().toggle_resize_mode();
            return true;
        case MoveUp:
            if (active_output && active_output->get_active_tree().try_move_active_window(Direction::up))
                return true;
            return false;
        case MoveDown:
            if (active_output && active_output->get_active_tree().try_move_active_window(Direction::down))
                return true;
            return false;
        case MoveLeft:
            if (active_output && active_output->get_active_tree().try_move_active_window(Direction::left))
                return true;
            return false;
        case MoveRight:
            if (active_output && active_output->get_active_tree().try_move_active_window(Direction::right))
                return true;
            return false;
        case SelectUp:
            if (active_output && (active_output->get_active_tree().try_resize_active_window(Direction::up)
                || active_output->get_active_tree().try_select_next(Direction::up)))
                return true;
            return false;
        case SelectDown:
            if (active_output && (active_output->get_active_tree().try_resize_active_window(Direction::down)
                || active_output->get_active_tree().try_select_next(Direction::down)))
                return true;
            return false;
        case SelectLeft:
            if (active_output && (active_output->get_active_tree().try_resize_active_window(Direction::left)
                || active_output->get_active_tree().try_select_next(Direction::left)))
                return true;
            return false;
        case SelectRight:
            if (active_output && (active_output->get_active_tree().try_resize_active_window(Direction::right)
                || active_output->get_active_tree().try_select_next(Direction::right)))
                return true;
            return false;
        case QuitActiveWindow:
            if (active_output) active_output->get_active_tree().close_active_window();
            return true;
        case QuitCompositor:
            runner.stop();
            return true;
        case Fullscreen:
            if (active_output) active_output->get_active_tree().try_toggle_active_fullscreen();
            return true;
        case SelectWorkspace1:
            if (active_output) workspace_manager.request_workspace(active_output, 1);
            break;
        case SelectWorkspace2:
            if (active_output) workspace_manager.request_workspace(active_output, 2);
            break;
        case SelectWorkspace3:
            if (active_output) workspace_manager.request_workspace(active_output, 3);
            break;
        case SelectWorkspace4:
            if (active_output) workspace_manager.request_workspace(active_output, 4);
            break;
        case SelectWorkspace5:
            if (active_output) workspace_manager.request_workspace(active_output, 5);
            break;
        case SelectWorkspace6:
            if (active_output) workspace_manager.request_workspace(active_output, 6);
            break;
        case SelectWorkspace7:
            if (active_output) workspace_manager.request_workspace(active_output, 7);
            break;
        case SelectWorkspace8:
            if (active_output) workspace_manager.request_workspace(active_output, 8);
            break;
        case SelectWorkspace9:
            if (active_output) workspace_manager.request_workspace(active_output, 9);
            break;
        case SelectWorkspace0:
            if (active_output) workspace_manager.request_workspace(active_output, 0);
            break;
        case MoveToWorkspace1:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 1);
            break;
        case MoveToWorkspace2:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 2);
            break;
        case MoveToWorkspace3:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 3);
            break;
        case MoveToWorkspace4:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 4);
            break;
        case MoveToWorkspace5:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 5);
            break;
        case MoveToWorkspace6:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 6);
            break;
        case MoveToWorkspace7:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 7);
            break;
        case MoveToWorkspace8:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 8);
            break;
        case MoveToWorkspace9:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 9);
            break;
        case MoveToWorkspace0:
            if (active_output) workspace_manager.move_active_to_workspace(active_output, 0);
            break;
        default:
            std::cerr << "Unknown key_command: " << key_command << std::endl;
            break;
    }
    return false;
}

bool MiracleWindowManagementPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);

    for (auto const& output : output_list)
    {
        if (output->point_is_in_output(static_cast<int>(x), static_cast<int>(y)))
        {
            if (active_output != output)
            {
                if (active_output) active_output->set_is_active(false);
                active_output = output;
                active_output->set_is_active(true);
                workspace_manager.request_focus(output->get_active_workspace());
            }

            if (output->get_active_workspace() >= 0)
            {
                active_output->get_active_tree().select_window_from_point(static_cast<int>(x), static_cast<int>(y));
            }
            break;
        }
    }

    return false;
}

auto MiracleWindowManagementPolicy::place_new_window(
    const miral::ApplicationInfo &app_info,
    const miral::WindowSpecification &requested_specification) -> miral::WindowSpecification
{
    if (!active_output)
    {
        mir::log_warning("place_new_window: no output available");
        return requested_specification;
    }

    pending_output = active_output;
    return active_output->get_active_tree().allocate_position(requested_specification);
}

void MiracleWindowManagementPolicy::_add_to_output_immediately(miral::Window& window, std::shared_ptr<Screen>& output)
{
    miral::WindowSpecification spec;
    spec = output->get_active_tree().allocate_position(spec);
    window_manager_tools.modify_window(window, spec);
    output->get_active_tree().advise_new_window(window_manager_tools.info_for(window));
}

void MiracleWindowManagementPolicy::advise_new_window(miral::WindowInfo const& window_info)
{
    auto shared_output = pending_output.lock();
    if (!shared_output)
    {
        mir::log_warning("advise_new_window: output unavailable");
        auto window = window_info.window();
        if (!output_list.empty())
        {
            // Our output is gone! Let's try to add it to a different output
            _add_to_output_immediately(window, output_list[0]);
        }
        else
        {
            // We have no output! Let's add it to a list of orphans
            orphaned_window_list.push_back(window);
            mir::log_info("Added an orphaned window");
        }

        return;
    }

    shared_output->get_active_tree().advise_new_window(window_info);
    pending_output.reset();
}

void MiracleWindowManagementPolicy::handle_window_ready(miral::WindowInfo &window_info)
{
    for (auto const& output : output_list)
    {
        if (output->get_active_tree().handle_window_ready(window_info))
            break;
    }
}

void MiracleWindowManagementPolicy::advise_focus_gained(const miral::WindowInfo &window_info)
{
    for (auto const& output : output_list)
        output->get_active_tree().advise_focus_gained(window_info.window());
    window_manager_tools.raise_tree(window_info.window());
}

void MiracleWindowManagementPolicy::advise_focus_lost(const miral::WindowInfo &window_info)
{
    for (auto const& output : output_list)
        output->get_active_tree().advise_focus_lost(window_info.window());
}

void MiracleWindowManagementPolicy::advise_delete_window(const miral::WindowInfo &window_info)
{
    for (auto it = orphaned_window_list.begin(); it != orphaned_window_list.end();)
    {
        if (*it == window_info.window())
            it = orphaned_window_list.erase(it);
        else
            it++;
    }

    for (auto const& output : output_list)
        output->get_active_tree().advise_delete_window(window_info.window());
}

void MiracleWindowManagementPolicy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
}

void MiracleWindowManagementPolicy::advise_output_create(miral::Output const& output)
{
    WindowTreeOptions options =  { config.get_gap_size_x(), config.get_gap_size_y() };
    auto new_tree = std::make_shared<Screen>(
        output, workspace_manager, output.extents(), window_manager_tools, options);
    workspace_manager.request_first_available_workspace(new_tree);
        output_list.push_back(new_tree);
    if (active_output == nullptr)
        active_output = new_tree;

    // Let's rehome some orphan windows if we need to
    if (!orphaned_window_list.empty())
    {
        for (auto& window : orphaned_window_list)
        {
            _add_to_output_immediately(window, active_output);
        }
        orphaned_window_list.clear();
    }
}

void MiracleWindowManagementPolicy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    for (auto& output : output_list)
    {
        if (output->get_output().is_same_output(original))
        {
            for (auto& workspace : output->get_workspaces())
            {
                workspace.tree.set_output_area(updated.extents());
            }
            break;
        }
    }
}

void MiracleWindowManagementPolicy::advise_output_delete(miral::Output const& output)
{
    for (auto it = output_list.begin(); it != output_list.end();)
    {
        auto other_output = *it;
        if (other_output->get_output().is_same_output(output))
        {
            it = output_list.erase(it);
            if (other_output == active_output)
            {
                if (output_list.empty())
                {
                    // All nodes should become orphaned
                    for (auto& workspace : other_output->get_workspaces())
                    {
                        workspace.tree.foreach_node([&](auto node)
                        {
                            if (node->is_window())
                            {
                                orphaned_window_list.push_back(node->get_window());
                            }
                        });
                    }

                    active_output = nullptr;
                }
                else
                {
                    active_output = output_list[0];
                    for (auto& workspace : other_output->get_workspaces())
                    {
                        active_output->get_active_tree().add_tree(workspace.tree);
                    }
                }
            }
            break;
        }
    }
}

void MiracleWindowManagementPolicy::advise_state_change(miral::WindowInfo const& window_info, MirWindowState state)
{
    for (auto const& output : output_list)
    {
        if (output->get_active_tree().advise_state_change(window_info, state))
        {
            break;
        }
    }
}

void MiracleWindowManagementPolicy::handle_modify_window(
    miral::WindowInfo &window_info,
    const miral::WindowSpecification &modifications)
{
    if (modifications.state().is_set())
    {
        if (modifications.state().value() == mir_window_state_fullscreen || modifications.state().value() == mir_window_state_maximized)
        {
            for (auto const& output : output_list)
            {
                bool found = false;
                for (auto& workspace : output->get_workspaces())
                {
                    if (workspace.tree.advise_fullscreen_window(window_info))
                    {
                        found = true;
                        break;
                    }
                }

                if (found) break;
            }
        }
        else if (modifications.state().value() == mir_window_state_restored)
        {
            for (auto const& output : output_list)
            {
                bool found = false;
                for (auto& workspace : output->get_workspaces())
                {
                    if (workspace.tree.advise_restored_window(window_info))
                    {
                        found = true;
                        break;
                    }
                }

                if (found) break;
            }
        }
    }

    for (auto const& output :output_list)
    {
        bool found = false;
        for (auto& workspace : output->get_workspaces())
        {
            if (workspace.tree.constrain(window_info))
            {
                found = true;
                window_manager_tools.modify_window(window_info.window(), modifications);
                break;
            }
        }

        if (found) break;
    }
}

void MiracleWindowManagementPolicy::handle_raise_window(miral::WindowInfo &window_info)
{
    window_manager_tools.select_active_window(window_info.window());
}

mir::geometry::Rectangle
MiracleWindowManagementPolicy::confirm_placement_on_display(
    const miral::WindowInfo &window_info,
    MirWindowState new_state,
    const mir::geometry::Rectangle &new_placement)
{
    mir::geometry::Rectangle modified_placement = new_placement;
    for (auto const& output : output_list)\
    {
        bool found = false;
        for (auto& workspace : output->get_workspaces())
        {
            if (workspace.tree.confirm_placement_on_display(window_info, new_state, modified_placement))
            {
                found = true;
                break;
            }
        }

        if (found) break;
    }
    return modified_placement;
}

bool MiracleWindowManagementPolicy::handle_touch_event(const MirTouchEvent *event)
{
    return false;
}

void MiracleWindowManagementPolicy::handle_request_move(miral::WindowInfo &window_info, const MirInputEvent *input_event)
{

}

void MiracleWindowManagementPolicy::handle_request_resize(
    miral::WindowInfo &window_info,
    const MirInputEvent *input_event,
    MirResizeEdge edge)
{

}

mir::geometry::Rectangle MiracleWindowManagementPolicy::confirm_inherited_move(
    const miral::WindowInfo &window_info,
    mir::geometry::Displacement movement)
{
    return {window_info.window().top_left()+movement, window_info.window().size()};
}

void MiracleWindowManagementPolicy::advise_application_zone_create(miral::Zone const& application_zone)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_create(application_zone);
    }
}

void MiracleWindowManagementPolicy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_update(updated, original);
    }
}

void MiracleWindowManagementPolicy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_delete(application_zone);
    }
}
