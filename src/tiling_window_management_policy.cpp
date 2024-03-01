#include "miral/window_specification.h"
#include "window_metadata.h"
#define MIR_LOG_COMPONENT "miracle"

#include "tiling_window_management_policy.h"
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
}

TilingWindowManagementPolicy::TilingWindowManagementPolicy(
    miral::WindowManagerTools const& tools,
    miral::ExternalClientLauncher const& external_client_launcher,
    miral::InternalClientLauncher const& internal_client_launcher,
    miral::MirRunner& runner,
    std::shared_ptr<MiracleConfig> const& config)
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
    config->register_listener([&](auto& new_config)
    {
        ipc->disconnect_all();
    }, 1);
}

TilingWindowManagementPolicy::~TilingWindowManagementPolicy()
{
    workspace_observer_registrar.unregister_interest(*ipc);
}

bool TilingWindowManagementPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

    auto custom_key_command = config->matches_custom_key_command(action, scan_code, modifiers);
    if (custom_key_command != nullptr)
    {
        external_client_launcher.launch(custom_key_command->command);
        return true;
    }

    auto key_command = config->matches_key_command(action, scan_code, modifiers);
    if (key_command == DefaultKeyCommand::MAX)
        return false;

    switch (key_command)
    {
        case Terminal:
        {
            auto terminal_command = config->get_terminal_command();
            if (terminal_command)
                external_client_launcher.launch({terminal_command.value()});
            return true;
        }
        case RequestVertical:
            if(active_output) active_output->get_active_tree()->request_vertical();
            return true;
        case RequestHorizontal:
            if(active_output) active_output->get_active_tree()->request_horizontal();
            return true;
        case ToggleResize:
            if(active_output) active_output->get_active_tree()->toggle_resize_mode();
            return true;
        case MoveUp:
            if (active_output && active_output->get_active_tree()->try_move_active_window(Direction::up))
                return true;
            return false;
        case MoveDown:
            if (active_output && active_output->get_active_tree()->try_move_active_window(Direction::down))
                return true;
            return false;
        case MoveLeft:
            if (active_output && active_output->get_active_tree()->try_move_active_window(Direction::left))
                return true;
            return false;
        case MoveRight:
            if (active_output && active_output->get_active_tree()->try_move_active_window(Direction::right))
                return true;
            return false;
        case SelectUp:
            if (active_output && (active_output->get_active_tree()->try_resize_active_window(Direction::up)
                || active_output->get_active_tree()->try_select_next(Direction::up)))
                return true;
            return false;
        case SelectDown:
            if (active_output && (active_output->get_active_tree()->try_resize_active_window(Direction::down)
                || active_output->get_active_tree()->try_select_next(Direction::down)))
                return true;
            return false;
        case SelectLeft:
            if (active_output && (active_output->get_active_tree()->try_resize_active_window(Direction::left)
                || active_output->get_active_tree()->try_select_next(Direction::left)))
                return true;
            return false;
        case SelectRight:
            if (active_output && (active_output->get_active_tree()->try_resize_active_window(Direction::right)
                || active_output->get_active_tree()->try_select_next(Direction::right)))
                return true;
            return false;
        case QuitActiveWindow:
            if (active_output) active_output->get_active_tree()->close_active_window();
            return true;
        case QuitCompositor:
            runner.stop();
            return true;
        case Fullscreen:
            if (active_output) active_output->get_active_tree()->try_toggle_active_fullscreen();
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

bool TilingWindowManagementPolicy::handle_pointer_event(MirPointerEvent const* event)
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
                active_output->get_active_tree()->select_window_from_point(static_cast<int>(x), static_cast<int>(y));
            }
            break;
        }
    }

    return false;
}

auto TilingWindowManagementPolicy::place_new_window(
    const miral::ApplicationInfo &app_info,
    const miral::WindowSpecification &requested_specification) -> miral::WindowSpecification
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

void TilingWindowManagementPolicy::_add_to_output_immediately(miral::Window& window, std::shared_ptr<OutputContent>& output)
{
    miral::WindowSpecification spec;
    spec = output->get_active_tree()->allocate_position(spec);
    window_manager_tools.modify_window(window, spec);
    output->get_active_tree()->advise_new_window(window_manager_tools.info_for(window));
}

void TilingWindowManagementPolicy::advise_new_window(miral::WindowInfo const& window_info)
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

    std::shared_ptr<WindowMetadata> metadata = nullptr;
    switch (pending_type)
    {
        case WindowType::tiled:
            metadata = shared_output->get_active_tree()->advise_new_window(window_info);
            break;
        case WindowType::other:
            if (window_info.state() == MirWindowState::mir_window_state_attached)
            {
                window_manager_tools.select_active_window(window_info.window());
            }
            metadata = std::make_shared<WindowMetadata>(WindowType::other, window_info.window());
            break;
        default:
            mir::log_error("Unsupported window type: %d", (int)pending_type);
            break;
    }

    if (metadata)
    {
        miral::WindowSpecification spec;
        spec.userdata() = metadata;
        window_manager_tools.modify_window(window_info.window(), spec);
    }
    else
    {
        mir::log_error("Window failed to set metadata");
    }
    
    pending_type = WindowType::none;
    pending_output.reset();
}

void TilingWindowManagementPolicy::handle_window_ready(miral::WindowInfo &window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
        return;

    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->handle_window_ready(window_info);
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void TilingWindowManagementPolicy::advise_focus_gained(const miral::WindowInfo &window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
        return;

    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->advise_focus_gained(window_info.window());
            window_manager_tools.raise_tree(window_info.window());
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void TilingWindowManagementPolicy::advise_focus_lost(const miral::WindowInfo &window_info)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
        return;

    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->advise_focus_lost(window_info.window());
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void TilingWindowManagementPolicy::advise_delete_window(const miral::WindowInfo &window_info)
{
    for (auto it = orphaned_window_list.begin(); it != orphaned_window_list.end();)
    {
        if (*it == window_info.window())
        {
            orphaned_window_list.erase(it);
            return;
        }
        else
            it++;
    }

    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::fatal_error("advise_delete_window: metadata is not provided");
        return;
    }

    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->advise_delete_window(window_info.window());
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void TilingWindowManagementPolicy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
}

void TilingWindowManagementPolicy::advise_output_create(miral::Output const& output)
{
    auto new_tree = std::make_shared<OutputContent>(
        output, workspace_manager, output.extents(), window_manager_tools, config);
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

void TilingWindowManagementPolicy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    for (auto& output : output_list)
    {
        if (output->get_output().is_same_output(original))
        {
            for (auto& workspace : output->get_workspaces())
            {
                workspace->get_tree()->set_output_area(updated.extents());
            }
            break;
        }
    }
}

void TilingWindowManagementPolicy::advise_output_delete(miral::Output const& output)
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
                        workspace->get_tree()->foreach_node([&](auto node)
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
                        active_output->get_active_tree()->add_tree(workspace->get_tree());
                    }
                }
            }
            break;
        }
    }
}

void TilingWindowManagementPolicy::advise_state_change(miral::WindowInfo const& window_info, MirWindowState state)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::fatal_error("advise_state_changed: metadata is not provided");
        return;
    }

    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            if (active_output->get_active_tree().get() != metadata->get_tiling_node()->get_tree())
                break;

            metadata->get_tiling_node()->get_tree()->advise_state_change(window_info, state);
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void TilingWindowManagementPolicy::handle_modify_window(
    miral::WindowInfo &window_info,
    const miral::WindowSpecification &modifications)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::fatal_error("handle_modify_window: metadata is not provided");
        return;
    }

    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            if (active_output->get_active_tree().get() != metadata->get_tiling_node()->get_tree())
                break;

            if (modifications.state().is_set())
            {
                if (modifications.state().value() == mir_window_state_fullscreen || modifications.state().value() == mir_window_state_maximized)
                    metadata->get_tiling_node()->get_tree()->advise_fullscreen_window(window_info);
                else if (modifications.state().value() == mir_window_state_restored)
                    metadata->get_tiling_node()->get_tree()->advise_restored_window(window_info);
            }

            metadata->get_tiling_node()->get_tree()->constrain(window_info);
            window_manager_tools.modify_window(window_info.window(), modifications);
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void TilingWindowManagementPolicy::handle_raise_window(miral::WindowInfo &window_info)
{
    window_manager_tools.select_active_window(window_info.window());
}

mir::geometry::Rectangle
TilingWindowManagementPolicy::confirm_placement_on_display(
    const miral::WindowInfo &window_info,
    MirWindowState new_state,
    const mir::geometry::Rectangle &new_placement)
{
    auto metadata = window_helpers::get_metadata(window_info);
    if (!metadata)
    {
        mir::log_error("confirm_placement_on_display: window lacks metadata");
        return new_placement;
    }

    mir::geometry::Rectangle modified_placement = new_placement;
    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->confirm_placement_on_display(window_info, new_state, modified_placement);
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            break;
    }

    return modified_placement;
}

bool TilingWindowManagementPolicy::handle_touch_event(const MirTouchEvent *event)
{
    return false;
}

void TilingWindowManagementPolicy::handle_request_move(miral::WindowInfo &window_info, const MirInputEvent *input_event)
{

}

void TilingWindowManagementPolicy::handle_request_resize(
    miral::WindowInfo &window_info,
    const MirInputEvent *input_event,
    MirResizeEdge edge)
{

}

mir::geometry::Rectangle TilingWindowManagementPolicy::confirm_inherited_move(
    const miral::WindowInfo &window_info,
    mir::geometry::Displacement movement)
{
    return { window_info.window().top_left() + movement, window_info.window().size() };
}

void TilingWindowManagementPolicy::advise_application_zone_create(miral::Zone const& application_zone)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_create(application_zone);
    }
}

void TilingWindowManagementPolicy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_update(updated, original);
    }
}

void TilingWindowManagementPolicy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    for (auto const& output : output_list)
    {
        output->advise_application_zone_delete(application_zone);
    }
}
