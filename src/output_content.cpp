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
#include "workspace_content.h"
#include <memory>
#define MIR_LOG_COMPONENT "output_content"

#include "animator.h"
#include "leaf_node.h"
#include "output_content.h"
#include "window_helpers.h"
#include "workspace_manager.h"
#include <glm/gtx/transform.hpp>
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>

using namespace miracle;

OutputContent::OutputContent(
    miral::Output const& output,
    WorkspaceManager& workspace_manager,
    geom::Rectangle const& area,
    miral::WindowManagerTools const& tools,
    miral::MinimalWindowManager& floating_window_manager,
    std::shared_ptr<MiracleConfig> const& config,
    TilingInterface& node_interface,
    Animator& animator) :
    output { output },
    workspace_manager { workspace_manager },
    area { area },
    tools { tools },
    floating_window_manager { floating_window_manager },
    config { config },
    node_interface { node_interface },
    animator { animator },
    handle { animator.register_animateable() }
{
}

std::shared_ptr<TilingWindowTree> const& OutputContent::get_active_tree() const
{
    return get_active_workspace()->get_tree();
}

std::shared_ptr<WorkspaceContent> const& OutputContent::get_active_workspace() const
{
    for (auto& info : workspaces)
    {
        if (info->get_workspace() == active_workspace)
            return info;
    }

    throw std::runtime_error("get_active_workspace: unable to find the active workspace. We shouldn't be here!");
}

bool OutputContent::handle_pointer_event(const MirPointerEvent* event)
{
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    if (get_active_workspace_num() < 0)
        return false;

    if (select_window_from_point(static_cast<int>(x), static_cast<int>(y)))
        return true;

    auto const action = mir_pointer_event_action(event);
    if (has_clicked_floating_window || get_active_workspace()->has_floating_window(active_window))
    {
        if (action == mir_pointer_action_button_down)
            has_clicked_floating_window = true;
        else if (action == mir_pointer_action_button_up)
            has_clicked_floating_window = false;
        return floating_window_manager.handle_pointer_event(event);
    }

    return false;
}

WindowType OutputContent::allocate_position(miral::ApplicationInfo const& app_info, miral::WindowSpecification& requested_specification)
{
    if (!window_helpers::is_tileable(requested_specification))
        return WindowType::other;

    auto type = get_active_workspace()->allocate_position(requested_specification);
    if (type == WindowType::floating)
    {
        requested_specification = floating_window_manager.place_new_window(app_info, requested_specification);
    }

    return type;
}

std::shared_ptr<WindowMetadata> OutputContent::advise_new_window(miral::WindowInfo const& window_info, WindowType type)
{
    std::shared_ptr<WindowMetadata> metadata = nullptr;
    switch (type)
    {
    case WindowType::tiled:
    {
        auto const& tree = get_active_tree();
        auto node = tree->advise_new_window(window_info);
        metadata = std::make_shared<WindowMetadata>(WindowType::tiled, window_info.window(), get_active_workspace());
        metadata->associate_to_node(node);
        break;
    }
    case WindowType::floating:
    {
        floating_window_manager.advise_new_window(window_info);
        metadata = std::make_shared<WindowMetadata>(WindowType::floating, window_info.window(), get_active_workspace());
        get_active_workspace()->add_floating_window(window_info.window());
        break;
    }
    case WindowType::other:
        if (window_info.state() == MirWindowState::mir_window_state_attached)
        {
            node_interface.select_active_window(window_info.window());
        }
        metadata = std::make_shared<WindowMetadata>(WindowType::other, window_info.window());
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)type);
        break;
    }

    if (metadata)
    {
        miral::WindowSpecification spec;
        spec.userdata() = metadata;
        spec.min_width() = mir::geometry::Width(0);
        spec.min_height() = mir::geometry::Height(0);
        tools.modify_window(window_info.window(), spec);

        // Warning: We need to advise fullscreen only after we've associated the userdata() appropriately
        if (type == WindowType::tiled && window_helpers::is_window_fullscreen(window_info.state()))
        {
            get_active_tree()->advise_fullscreen_window(window_info.window());
        }
        return metadata;
    }
    else
    {
        mir::log_error("Window failed to set metadata");
        return nullptr;
    }
}

void OutputContent::handle_window_ready(miral::WindowInfo& window_info, std::shared_ptr<miracle::WindowMetadata> const& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        auto tree = metadata->get_tiling_node()->get_tree();
        tree->handle_window_ready(window_info);

        // Note: By default, new windows are raised. To properly maintain the ordering, we must
        // raise floating windows and then raise fullscreen windows.
        for (auto const& window : get_active_workspace()->get_floating_windows())
            node_interface.raise(window);

        if (tree->has_fullscreen_window())
            node_interface.raise(active_window);
        break;
    }
    case WindowType::floating:
        floating_window_manager.handle_window_ready(window_info);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::advise_focus_gained(const std::shared_ptr<miracle::WindowMetadata>& metadata)
{
    active_window = metadata->get_window();
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        metadata->get_tiling_node()->get_tree()->advise_focus_gained(metadata->get_window());
        break;
    }
    case WindowType::floating:
        floating_window_manager.advise_focus_gained(tools.info_for(metadata->get_window()));
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::advise_focus_lost(const std::shared_ptr<miracle::WindowMetadata>& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        metadata->get_tiling_node()->get_tree()->advise_focus_lost(metadata->get_window());
        break;
    }
    case WindowType::floating:
        floating_window_manager.advise_focus_lost(tools.info_for(metadata->get_window()));
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::advise_delete_window(const std::shared_ptr<miracle::WindowMetadata>& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        metadata->get_tiling_node()->get_tree()->advise_delete_window(metadata->get_window());
        break;
    }
    case WindowType::floating:
        floating_window_manager.advise_delete_window(tools.info_for(metadata->get_window()));
        // TODO: There really should be a mapping from a floating window back to its workspace
        for (auto& workspace : workspaces)
        {
            if (workspace->has_floating_window(metadata->get_window()))
            {
                workspace->remove_floating_window(metadata->get_window());
                break;
            }
        }
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::advise_move_to(std::shared_ptr<miracle::WindowMetadata> const& metadata, geom::Point top_left)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        break;
    case WindowType::floating:
        floating_window_manager.advise_move_to(tools.info_for(metadata->get_window()), top_left);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::handle_request_move(const std::shared_ptr<miracle::WindowMetadata>& metadata,
    const MirInputEvent* input_event)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        break;
    case WindowType::floating:
        floating_window_manager.handle_request_move(tools.info_for(metadata->get_window()), input_event);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::handle_request_resize(
    const std::shared_ptr<miracle::WindowMetadata>& metadata,
    const MirInputEvent* input_event,
    MirResizeEdge edge)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        break;
    case WindowType::floating:
        floating_window_manager.handle_request_resize(tools.info_for(metadata->get_window()), input_event, edge);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::advise_state_change(const std::shared_ptr<miracle::WindowMetadata>& metadata, MirWindowState state)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        break;
    case WindowType::floating:
        if (!get_active_workspace()->has_floating_window(metadata->get_window()))
            break;

        floating_window_manager.advise_state_change(tools.info_for(metadata->get_window()), state);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::handle_modify_window(const std::shared_ptr<miracle::WindowMetadata>& metadata,
    const miral::WindowSpecification& modifications)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        auto& window = metadata->get_window();
        auto node = metadata->get_tiling_node();
        auto const& info = tools.info_for(window);
        if (get_active_tree().get() != node->get_tree())
            break;

        if (modifications.state().is_set() && modifications.state().value() != info.state())
        {
            auto tree = metadata->get_tiling_node()->get_tree();
            node->set_state(modifications.state().value());
            node->commit_changes();

            if (window_helpers::is_window_fullscreen(modifications.state().value()))
                tree->advise_fullscreen_window(window);
            else if (modifications.state().value() == mir_window_state_restored)
                tree->advise_restored_window(window);
        }

        tools.modify_window(window, modifications);
        break;
    }
    case WindowType::floating:
        if (!get_active_workspace()->has_floating_window(metadata->get_window()))
            break;

        floating_window_manager.handle_modify_window(tools.info_for(metadata->get_window()), modifications);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::handle_raise_window(const std::shared_ptr<miracle::WindowMetadata>& metadata)
{
    switch (metadata->get_type())
    {
    case WindowType::tiled:
        node_interface.select_active_window(metadata->get_window());
        break;
    case WindowType::floating:
        floating_window_manager.handle_raise_window(tools.info_for(metadata->get_window()));
        break;
    default:
        mir::log_error("handle_raise_window: unsupported window type: %d", (int)metadata->get_type());
        return;
    }
}

mir::geometry::Rectangle
OutputContent::confirm_placement_on_display(
    const std::shared_ptr<miracle::WindowMetadata>& metadata,
    MirWindowState new_state,
    const mir::geometry::Rectangle& new_placement)
{
    mir::geometry::Rectangle modified_placement = new_placement;
    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        metadata->get_tiling_node()->get_tree()->confirm_placement_on_display(
            metadata->get_window(), new_state, modified_placement);
        break;
    }
    case WindowType::floating:
        return floating_window_manager.confirm_placement_on_display(tools.info_for(metadata->get_window()), new_state, new_placement);
    default:
        mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
        break;
    }
    return modified_placement;
}

bool OutputContent::select_window_from_point(int x, int y)
{
    auto const& workspace = get_active_workspace();
    if (workspace->get_tree()->has_fullscreen_window())
        return false;

    auto const& floating = workspace->get_floating_windows();
    int floating_index = -1;
    for (int i = 0; i < floating.size(); i++)
    {
        geom::Rectangle window_area(floating[i].top_left(), floating[i].size());
        if (floating[i] == active_window && window_area.contains(geom::Point(x, y)))
            return false;
        else if (window_area.contains(geom::Point(x, y)))
            floating_index = i;
    }

    if (floating_index >= 0)
    {
        node_interface.select_active_window(floating[floating_index]);
        return true;
    }

    auto node = workspace->get_tree()->select_window_from_point(x, y);
    if (node && node->get_window() != active_window)
    {
        node_interface.select_active_window(node->get_window());
        return true;
    }

    return false;
}

void OutputContent::select_window(miral::Window const& window)
{
    node_interface.select_active_window(window);
}

namespace
{
template <typename T, typename Pred>
typename std::vector<T>::iterator
insert_sorted(std::vector<T>& vec, T const& item, Pred pred)
{
    return vec.insert(
        std::upper_bound(vec.begin(), vec.end(), item, pred),
        item);
}
}

void OutputContent::advise_new_workspace(int workspace)
{
    // Workspaces are always kept in sorted order
    auto new_workspace = std::make_shared<WorkspaceContent>(this, tools, workspace, config, node_interface);
    insert_sorted(workspaces, new_workspace, [](std::shared_ptr<WorkspaceContent> const& a, std::shared_ptr<WorkspaceContent> const& b)
    {
        return a->get_workspace() < b->get_workspace();
    });
}

void OutputContent::advise_workspace_deleted(int workspace)
{
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (it->get()->get_workspace() == workspace)
        {
            workspaces.erase(it);
            return;
        }
    }
}

bool OutputContent::advise_workspace_active(int key)
{
    int from_index = -1, to_index = -1;
    std::shared_ptr<WorkspaceContent> from = nullptr;
    std::shared_ptr<WorkspaceContent> to = nullptr;
    for (int i = 0; i < workspaces.size(); i++)
    {
        auto const& workspace = workspaces[i];
        if (workspace->get_workspace() == active_workspace)
        {
            from = workspace;
            from_index = i;
        }

        if (workspace->get_workspace() == key)
        {
            if (active_workspace == key)
                return true;

            to = workspace;
            to_index = i;
        }
    }

    if (!to)
    {
        mir::fatal_error("advise_workspace_active: switch to workspace that doesn't exist: %d", key);
        return false;
    }

    if (!from)
    {
        to->show();
        active_workspace = key;

        auto to_rectangle = get_workspace_rectangle(active_workspace);
        set_position(glm::vec2(
            to_rectangle.top_left.x.as_int(),
            to_rectangle.top_left.y.as_int()));
        to->trigger_rerender();
        return true;
    }

    auto from_src = get_workspace_rectangle(from->get_workspace());
    from->transfer_pinned_windows_to(to);

    // If 'from' is empty, we can delete the workspace. However, this means that from_src is now incorrect
    if (from->is_empty())
        workspace_manager.delete_workspace(from->get_workspace());

    // Show everyone so that we can animate over all workspaces
    for (auto const& workspace : workspaces)
        workspace->show();

    geom::Rectangle real {
        { geom::X { position_offset.x }, geom::Y { position_offset.y } },
        area.size
    };
    auto to_src = get_workspace_rectangle(to->get_workspace());
    geom::Rectangle src {
        { geom::X { -from_src.top_left.x.as_int() }, geom::Y { from_src.top_left.y.as_int() } },
        area.size
    };
    geom::Rectangle dest {
        { geom::X { -to_src.top_left.x.as_int() }, geom::Y { to_src.top_left.y.as_int() } },
        area.size
    };
    animator.workspace_switch(
        handle,
        src,
        dest,
        real,
        [this, to = to, from = from](AnimationStepResult const& asr)
    {
        if (asr.is_complete)
        {
            if (asr.position)
                set_position(asr.position.value());
            if (asr.transform)
                set_transform(asr.transform.value());

            for (auto const& workspace : workspaces)
            {
                if (workspace != to)
                    workspace->hide();
            }

            to->trigger_rerender();
            return;
        }

        if (asr.position)
            set_position(asr.position.value());
        if (asr.transform)
            set_transform(asr.transform.value());

        for (auto const& workspace : workspaces)
            workspace->trigger_rerender();
    });

    active_workspace = key;
    return true;
}

void OutputContent::advise_application_zone_create(miral::Zone const& application_zone)
{
    if (application_zone.extents().contains(area))
    {
        application_zone_list.push_back(application_zone);
        for (auto& workspace : workspaces)
            workspace->get_tree()->recalculate_root_node_area();
    }
}

void OutputContent::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto& zone : application_zone_list)
        if (zone == original)
        {
            zone = updated;
            for (auto& workspace : workspaces)
                workspace->get_tree()->recalculate_root_node_area();
            break;
        }
}

void OutputContent::advise_application_zone_delete(miral::Zone const& application_zone)
{
    if (std::remove(application_zone_list.begin(), application_zone_list.end(), application_zone) != application_zone_list.end())
    {
        for (auto& workspace : workspaces)
            workspace->get_tree()->recalculate_root_node_area();
    }
}

bool OutputContent::point_is_in_output(int x, int y)
{
    return area.contains(geom::Point(x, y));
}

void OutputContent::close_active_window()
{
    tools.ask_client_to_close(active_window);
}

bool OutputContent::resize_active_window(miracle::Direction direction)
{
    return get_active_tree()->try_resize_active_window(direction);
}

bool OutputContent::select(miracle::Direction direction)
{
    return get_active_tree()->try_select_next(direction);
}

bool OutputContent::move_active_window(miracle::Direction direction)
{
    auto metadata = window_helpers::get_metadata(active_window, tools);
    if (!metadata)
        return false;

    switch (metadata->get_type())
    {
    case WindowType::floating:
        return move_active_window_by_amount(direction, 10);
    case WindowType::tiled:
        return get_active_tree()->try_move_active_window(direction);
    default:
        mir::log_error("move_active_window is not defined for window of type %d", (int)metadata->get_type());
        return false;
    }
}

bool OutputContent::move_active_window_by_amount(Direction direction, int pixels)
{
    auto metadata = window_helpers::get_metadata(active_window, tools);
    if (!metadata)
        return false;

    if (metadata->get_type() != WindowType::floating)
    {
        mir::log_warning("Cannot move a non-floating window by an amount, type=%d", (int)metadata->get_type());
        return false;
    }

    auto& info = tools.info_for(active_window);
    auto prev_pos = active_window.top_left();
    miral::WindowSpecification spec;
    switch (direction)
    {
    case Direction::down:
        spec.top_left() = {
            prev_pos.x.as_int(), prev_pos.y.as_int() + pixels
        };
        break;
    case Direction::up:
        spec.top_left() = {
            prev_pos.x.as_int(), prev_pos.y.as_int() - pixels
        };
        break;
    case Direction::left:
        spec.top_left() = {
            prev_pos.x.as_int() - pixels, prev_pos.y.as_int()
        };
        break;
    case Direction::right:
        spec.top_left() = {
            prev_pos.x.as_int() + pixels, prev_pos.y.as_int()
        };
        break;
    default:
        mir::log_warning("Unknown direction to move_active_window_by_amount: %d\n", (int)direction);
        return false;
    }

    tools.modify_window(info, spec);
    return true;
}

bool OutputContent::move_active_window_to(int x, int y)
{
    auto metadata = window_helpers::get_metadata(active_window, tools);
    if (!metadata)
        return false;

    if (metadata->get_type() != WindowType::floating)
    {
        mir::log_warning("Cannot move a non-floating window to a position, type=%d", (int)metadata->get_type());
        return false;
    }

    miral::WindowSpecification spec;
    spec.top_left() = {
        x,
        y,
    };
    tools.modify_window(tools.info_for(active_window), spec);
    return true;
}

void OutputContent::request_vertical()
{
    get_active_tree()->request_vertical();
}

void OutputContent::request_horizontal()
{
    get_active_tree()->request_horizontal();
}

void OutputContent::toggle_layout()
{
    get_active_tree()->toggle_layout();
}

void OutputContent::toggle_fullscreen()
{
    get_active_tree()->try_toggle_active_fullscreen();
}

void OutputContent::toggle_pinned_to_workspace()
{
    auto metadata = window_helpers::get_metadata(tools.active_window(), tools);
    if (!metadata)
    {
        mir::log_error("toggle_pinned_to_workspace: metadata not found");
        return;
    }

    metadata->toggle_pin_to_desktop();
}

void OutputContent::set_is_pinned(bool is_pinned)
{
    auto metadata = window_helpers::get_metadata(tools.active_window(), tools);
    if (!metadata)
    {
        mir::log_error("set_is_pinned: metadata not found");
        return;
    }

    metadata->set_is_pinned(is_pinned);
}

void OutputContent::update_area(geom::Rectangle const& new_area)
{
    area = new_area;
    for (auto& workspace : workspaces)
    {
        workspace->get_tree()->set_output_area(area);
    }
}

std::vector<miral::Window> OutputContent::collect_all_windows() const
{
    std::vector<miral::Window> windows;
    for (auto& workspace : get_workspaces())
    {
        workspace->get_tree()->foreach_node([&](auto const& node)
        {
            auto leaf_node = Node::as_leaf(node);
            if (leaf_node)
                windows.push_back(leaf_node->get_window());
        });
    }
    return windows;
}

void OutputContent::request_toggle_active_float()
{
    if (tools.active_window() == Window())
    {
        mir::log_warning("request_toggle_active_float: active window unset");
        return;
    }

    auto metadata = window_helpers::get_metadata(tools.active_window(), tools);
    if (!metadata)
    {
        mir::log_error("request_toggle_active_float: metadata not found");
        return;
    }

    switch (metadata->get_type())
    {
    case WindowType::tiled:
    {
        auto tree = metadata->get_tiling_node()->get_tree();
        if (tree->has_fullscreen_window())
        {
            mir::log_warning("request_toggle_active_float: cannot float fullscreen window");
            return;
        }

        tree->advise_delete_window(active_window);

        auto& prev_info = tools.info_for(active_window);
        WindowSpecification prev_spec = window_helpers::copy_from(prev_info);

        auto& info = tools.info_for(active_window);
        info.clip_area(area);

        WindowSpecification spec = floating_window_manager.place_new_window(
            tools.info_for(active_window.application()),
            prev_spec);
        spec.userdata() = std::make_shared<WindowMetadata>(WindowType::floating, active_window, get_active_workspace());
        spec.top_left() = geom::Point { active_window.top_left().x.as_int() + 20, active_window.top_left().y.as_int() + 20 };
        tools.modify_window(active_window, spec);

        advise_new_window(info, WindowType::floating);
        auto new_metadata = window_helpers::get_metadata(active_window, tools);
        handle_window_ready(info, new_metadata);
        node_interface.select_active_window(active_window);
        get_active_workspace()->add_floating_window(active_window);
        break;
    }
    case WindowType::floating:
    {
        advise_delete_window(window_helpers::get_metadata(active_window, tools));
        add_immediately(active_window);
        break;
    }
    default:
        mir::log_warning("request_toggle_active_float: has no effect on window of type: %d", (int)metadata->get_type());
        return;
    }
}

void OutputContent::add_immediately(miral::Window& window)
{
    auto& prev_info = tools.info_for(window);
    WindowSpecification spec = window_helpers::copy_from(prev_info);
    WindowType type = allocate_position(tools.info_for(window.application()), spec);
    tools.modify_window(window, spec);
    advise_new_window(tools.info_for(window), type);
    auto metadata = window_helpers::get_metadata(window, tools);
    handle_window_ready(tools.info_for(window), metadata);
}

geom::Rectangle OutputContent::get_workspace_rectangle(int workspace) const
{
    // TODO: Support vertical workspaces one day in the future
    size_t x = (WorkspaceContent::workspace_to_number(workspace)) * area.size.width.as_int();
    return geom::Rectangle {
        geom::Point { geom::X { x },            geom::Y { 0 }             },
        geom::Size { area.size.width.as_int(), area.size.height.as_int() }
    };
}

glm::mat4 OutputContent::get_transform() const
{
    return final_transform;
}

void OutputContent::set_transform(glm::mat4 const& in)
{
    transform = in;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

void OutputContent::set_position(glm::vec2 const& v)
{
    position_offset = v;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

glm::vec2 const& OutputContent::get_position() const
{
    return position_offset;
}
