#define MIR_LOG_COMPONENT "miracle"

#include "miracle_window_management_policy.h"

#include <mir_toolkit/events/enums.h>
#include <miral/toolkit_event.h>
#include <miral/application_info.h>
#include <miral/zone.h>
#include <mir/log.h>
#include <linux/input.h>
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

const std::string TERMINAL = "konsole";

template <typename T>
bool is_tileable(T& requested_specification)
{
    auto t = requested_specification.type();
    auto state = requested_specification.state();
    auto has_exclusive_rect = requested_specification.exclusive_rect().is_set();
    return (t == mir_window_type_normal || t == mir_window_type_freestyle)
        && (state == mir_window_state_restored || state == mir_window_state_maximized)
        && !has_exclusive_rect;
}
}

MiracleWindowManagementPolicy::MiracleWindowManagementPolicy(
    const miral::WindowManagerTools & tools,
    miral::ExternalClientLauncher const& external_client_launcher,
    miral::InternalClientLauncher const& internal_client_launcher)
    : window_manager_tools{tools},
      external_client_launcher{external_client_launcher},
      internal_client_launcher{internal_client_launcher}
{
}

bool MiracleWindowManagementPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

    if (action == MirKeyboardAction::mir_keyboard_action_down && (modifiers & mir_input_event_modifier_alt))
    {
        if (scan_code == KEY_ENTER)
        {
            external_client_launcher.launch({TERMINAL});
            return true;
        }
        else if (scan_code == KEY_V)
        {
            active_tree->tree.request_vertical();
            return true;
        }
        else if (scan_code == KEY_H)
        {
            active_tree->tree.request_horizontal();
            return true;
        }
        else if (scan_code == KEY_R)
        {
            active_tree->tree.toggle_resize_mode();
            return true;
        }
        else if (scan_code == KEY_UP)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (active_tree->tree.try_move_active_window(Direction::up))
                    return true;
            }
            else if (active_tree->tree.try_resize_active_window(Direction::up))
                return true;
            else if (active_tree->tree.try_select_next(Direction::up))
                return true;
        }
        else if (scan_code == KEY_DOWN)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (active_tree->tree.try_move_active_window(Direction::down))
                    return true;
            }
            else if (active_tree->tree.try_resize_active_window(Direction::down))
                return true;
            else if (active_tree->tree.try_select_next(Direction::down))
                return true;
        }
        else if (scan_code == KEY_LEFT)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (active_tree->tree.try_move_active_window(Direction::left))
                    return true;
            }
            else if (active_tree->tree.try_resize_active_window(Direction::left))
                return true;
            else if (active_tree->tree.try_select_next(Direction::left))
                return true;
        }
        else if (scan_code == KEY_RIGHT)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (active_tree->tree.try_move_active_window(Direction::right))
                    return true;
            }
            else if (active_tree->tree.try_resize_active_window(Direction::right))
                return true;
            else if (active_tree->tree.try_select_next(Direction::right))
                return true;
        }
    }

    return false;
}

bool MiracleWindowManagementPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);

    for (auto pair : tree_list)
    {
        if (pair->tree.point_is_in_output(x, y))
        {
            active_tree = pair;
            active_tree->tree.select_window_from_point(x, y);
            break;
        }
    }

    return false;
}

auto MiracleWindowManagementPolicy::place_new_window(
    const miral::ApplicationInfo &app_info,
    const miral::WindowSpecification &requested_specification) -> miral::WindowSpecification
{
    if (is_tileable(requested_specification))
    {
        // In this step, we'll ask the WindowTree where we should place the window on the display
        // We will also resize the adjacent windows accordingly in this step.
        return active_tree->tree.allocate_position(requested_specification);
    }

    return requested_specification;
}

void MiracleWindowManagementPolicy::advise_new_window(miral::WindowInfo const& window_info)
{
    miral::WindowManagementPolicy::advise_new_window(window_info);
    if (is_tileable(window_info))
        active_tree->tree.advise_new_window(window_info);
}

void MiracleWindowManagementPolicy::handle_window_ready(miral::WindowInfo &window_info)
{
    if (!is_tileable(window_info))
    {
        return;
    }

    if (window_info.can_be_active())
        window_manager_tools.select_active_window(window_info.window());
}

void MiracleWindowManagementPolicy::advise_focus_gained(const miral::WindowInfo &window_info)
{
    for (auto tree : tree_list)
        tree->tree.advise_focus_gained(window_info.window());
    window_manager_tools.raise_tree(window_info.window());
}

void MiracleWindowManagementPolicy::advise_focus_lost(const miral::WindowInfo &window_info)
{
    for (auto tree : tree_list)
        tree->tree.advise_focus_lost(window_info.window());
}

void MiracleWindowManagementPolicy::advise_delete_window(const miral::WindowInfo &window_info)
{
    for (auto tree : tree_list)
        tree->tree.advise_delete_window(window_info.window());
}

void MiracleWindowManagementPolicy::advise_resize(miral::WindowInfo const& window_info, geom::Size const& new_size)
{
    for (auto tree : tree_list)
        tree->tree.advise_resize(window_info, new_size);
}

void MiracleWindowManagementPolicy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
    miral::WindowManagementPolicy::advise_move_to(window_info, top_left);
}

void MiracleWindowManagementPolicy::advise_output_create(miral::Output const& output)
{
    WindowTreeOptions options =  { 10, 10 };
    auto new_tree = std::make_shared<OutputTreePair>(
        output,
        WindowTree(output.extents(), window_manager_tools, options));
    tree_list.push_back(new_tree);
    if (active_tree == nullptr)
        active_tree = new_tree;
}

void MiracleWindowManagementPolicy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    for (auto& pair : tree_list)
    {
        if (pair->output.is_same_output(original))
        {
            pair->tree.set_output_area(updated.extents());
            break;
        }
    }
}

void MiracleWindowManagementPolicy::advise_output_delete(miral::Output const& output)
{
    for (auto it : tree_list)
    {
       if (it->output.is_same_output(output))
       {
           // TODO: Move windows open on the dying output to the other output
            std::remove(tree_list.begin(), tree_list.end(), it);
            if (it == active_tree)
            {
                if (tree_list.empty())
                    active_tree = nullptr;
                else
                    active_tree = tree_list[0];
            }
            break;
       }
    }
}

void MiracleWindowManagementPolicy::handle_modify_window(
    miral::WindowInfo &window_info,
    const miral::WindowSpecification &modifications)
{
    window_manager_tools.modify_window(window_info.window(), modifications);
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
    return new_placement;
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
    for (auto tree : tree_list)
        tree->tree.advise_application_zone_create(application_zone);
}

void MiracleWindowManagementPolicy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto tree : tree_list)
        tree->tree.advise_application_zone_update(updated, original);
}

void MiracleWindowManagementPolicy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    for (auto tree : tree_list)
        tree->tree.advise_application_zone_delete(application_zone);
}
