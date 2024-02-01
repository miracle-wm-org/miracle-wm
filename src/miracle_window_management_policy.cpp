#define MIR_LOG_COMPONENT "miracle"

#include "miracle_window_management_policy.h"
#include "window_helpers.h"
#include "miracle_config.h"

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
    miral::InternalClientLauncher const& internal_client_launcher,
    miral::MirRunner& runner)
    : config{MiracleConfig()},
      window_manager_tools{tools},
      external_client_launcher{external_client_launcher},
      internal_client_launcher{internal_client_launcher},
      runner{runner}
{
}

bool MiracleWindowManagementPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

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
            active_tree->tree.request_vertical();
            return true;
        case RequestHorizontal:
            active_tree->tree.request_horizontal();
            return true;
        case ToggleResize:
            active_tree->tree.toggle_resize_mode();
            return true;
        case MoveUp:
            if (active_tree->tree.try_move_active_window(Direction::up))
                return true;
            return false;
        case MoveDown:
            if (active_tree->tree.try_move_active_window(Direction::down))
                return true;
            return false;
        case MoveLeft:
            if (active_tree->tree.try_move_active_window(Direction::left))
                return true;
            return false;
        case MoveRight:
            if (active_tree->tree.try_move_active_window(Direction::right))
                return true;
            return false;
        case SelectUp:
            if (active_tree->tree.try_resize_active_window(Direction::up)
                || active_tree->tree.try_select_next(Direction::up))
                return true;
            return false;
        case SelectDown:
            if (active_tree->tree.try_resize_active_window(Direction::down)
                || active_tree->tree.try_select_next(Direction::down))
                return true;
            return false;
        case SelectLeft:
            if (active_tree->tree.try_resize_active_window(Direction::left)
                || active_tree->tree.try_select_next(Direction::left))
                return true;
            return false;
        case SelectRight:
            if (active_tree->tree.try_resize_active_window(Direction::right)
                || active_tree->tree.try_select_next(Direction::right))
                return true;
            return false;
        case QuitActiveWindow:
            active_tree->tree.close_active_window();
            return true;
        case QuitCompositor:
            runner.stop();
            return true;
        case Fullscreen:
            active_tree->tree.try_toggle_active_fullscreen();
            return true;
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

    for (auto const& pair : tree_list)
    {
        if (pair->tree.point_is_in_output(static_cast<int>(x), static_cast<int>(y)))
        {
            active_tree = pair;
            active_tree->tree.select_window_from_point(static_cast<int>(x), static_cast<int>(y));
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

    for (auto const& tree : tree_list)
    {
        if (tree->tree.handle_window_ready(window_info))
            break;
    }
}

void MiracleWindowManagementPolicy::advise_focus_gained(const miral::WindowInfo &window_info)
{
    for (auto const& tree : tree_list)
        tree->tree.advise_focus_gained(window_info.window());
    window_manager_tools.raise_tree(window_info.window());
}

void MiracleWindowManagementPolicy::advise_focus_lost(const miral::WindowInfo &window_info)
{
    for (auto const& tree : tree_list)
        tree->tree.advise_focus_lost(window_info.window());
}

void MiracleWindowManagementPolicy::advise_delete_window(const miral::WindowInfo &window_info)
{
    for (auto const& tree : tree_list)
        tree->tree.advise_delete_window(window_info.window());
}

void MiracleWindowManagementPolicy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
    miral::WindowManagementPolicy::advise_move_to(window_info, top_left);
}

void MiracleWindowManagementPolicy::advise_output_create(miral::Output const& output)
{
    WindowTreeOptions options =  { config.get_gap_size_x(), config.get_gap_size_y() };
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
    for (auto const& it : tree_list)
    {
       if (it->output.is_same_output(output))
       {
           // TODO: Move windows open on the dying output to the other output
            auto did_remove = std::remove(tree_list.begin(), tree_list.end(), it);
            if (did_remove != tree_list.end() && it == active_tree)
            {
                if (tree_list.empty())
                    active_tree = nullptr;
                else
                {
                    active_tree = tree_list[0];
                    active_tree->tree.add_tree(it->tree);
                }
            }
            break;
       }
    }
}

void MiracleWindowManagementPolicy::advise_state_change(miral::WindowInfo const& window_info, MirWindowState state)
{
    for (auto const& tree : tree_list)
    {
        if (tree->tree.advise_state_change(window_info, state))
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
            for (auto const& tree : tree_list)
                if (tree->tree.advise_fullscreen_window(window_info))
                    break;
        }
        else if (modifications.state().value() == mir_window_state_restored)
        {
            for (auto const& tree : tree_list)
            {
                if (tree->tree.advise_restored_window(window_info))
                    break;
            }
        }
    }

    for (auto const& tree :tree_list)
    {
        if (tree->tree.constrain(window_info))
            break;
    }

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
    mir::geometry::Rectangle modified_placement = new_placement;
    for (auto const& tree : tree_list) {

        if (tree->tree.confirm_placement_on_display(window_info, new_state, modified_placement))
        {
            break;
        }
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
    for (auto const& tree : tree_list)
        tree->tree.advise_application_zone_create(application_zone);
}

void MiracleWindowManagementPolicy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto const& tree : tree_list)
        tree->tree.advise_application_zone_update(updated, original);
}

void MiracleWindowManagementPolicy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    for (auto const& tree : tree_list)
        tree->tree.advise_application_zone_delete(application_zone);
}
