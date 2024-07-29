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

#define MIR_LOG_COMPONENT "output_content"
#define GLM_ENABLE_EXPERIMENTAL

#include "output.h"
#include "animator.h"
#include "compositor_state.h"
#include "leaf_container.h"
#include "floating_container.h"
#include "vector_helpers.h"
#include "window_helpers.h"

#include "workspace.h"
#include "workspace_manager.h"
#include <glm/gtx/transform.hpp>
#include <memory>
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>

using namespace miracle;

Output::Output(
    miral::Output const& output,
    WorkspaceManager& workspace_manager,
    geom::Rectangle const& area,
    miral::WindowManagerTools const& tools,
    miral::MinimalWindowManager& floating_window_manager,
    CompositorState& state,
    std::shared_ptr<MiracleConfig> const& config,
    WindowController& node_interface,
    Animator& animator) :
    output { output },
    workspace_manager { workspace_manager },
    area { area },
    tools { tools },
    floating_window_manager { floating_window_manager },
    state { state },
    config { config },
    window_controller { node_interface },
    animator { animator },
    handle { animator.register_animateable() }
{
}

std::shared_ptr<Workspace> const& Output::get_active_workspace() const
{
    for (auto& info : workspaces)
    {
        if (info->get_workspace() == active_workspace)
            return info;
    }

    throw std::runtime_error("get_active_workspace: unable to find the active workspace. We shouldn't be here!");
}

bool Output::handle_pointer_event(const MirPointerEvent* event)
{
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    if (get_active_workspace_num() < 0)
        return false;

    if (select_window_from_point(static_cast<int>(x), static_cast<int>(y)))
        return true;

    auto const action = mir_pointer_event_action(event);
    if (has_clicked_floating_window || get_active_workspace()->has_floating_window(state.active_window))
    {
        if (action == mir_pointer_action_button_down)
            has_clicked_floating_window = true;
        else if (action == mir_pointer_action_button_up)
            has_clicked_floating_window = false;
        return floating_window_manager.handle_pointer_event(event);
    }

    return false;
}

ContainerType Output::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    ContainerType hint)
{
    auto ideal_type = hint == ContainerType::none ? window_helpers::get_ideal_type(requested_specification) : hint;
    if (ideal_type == ContainerType::shell)
        return ContainerType::shell;

    return get_active_workspace()->allocate_position(app_info, requested_specification, ideal_type);
}

std::shared_ptr<Container> Output::advise_new_window(
    miral::WindowInfo const& window_info, ContainerType type) const
{
    return get_active_workspace()->advise_new_window(window_info, type);
}

void Output::handle_window_ready(
    miral::WindowInfo& window_info, std::shared_ptr<miracle::Container> const& metadata) const
{
    get_active_workspace()->handle_window_ready(window_info, metadata);
}

void Output::advise_focus_gained(const std::shared_ptr<miracle::Container>& metadata)
{
    auto workspace = metadata->get_workspace();
    workspace->advise_focus_gained(metadata);
}

void Output::advise_focus_lost(const std::shared_ptr<miracle::Container>& metadata)
{
    auto workspace = metadata->get_workspace();
    workspace->advise_focus_lost(metadata);
}

void Output::advise_delete_window(const std::shared_ptr<miracle::Container>& metadata)
{
    auto workspace = metadata->get_workspace();
    workspace->advise_delete_window(metadata);
}

void Output::advise_move_to(std::shared_ptr<miracle::Container> const& metadata, geom::Point top_left)
{
    auto workspace = metadata->get_workspace();
    workspace->advise_move_to(metadata, top_left);
}

void Output::handle_request_move(const std::shared_ptr<miracle::Container>& metadata,
    const MirInputEvent* input_event)
{
    auto workspace = metadata->get_workspace();
    workspace->handle_request_move(metadata, input_event);
}

void Output::handle_request_resize(
    const std::shared_ptr<miracle::Container>& metadata,
    const MirInputEvent* input_event,
    MirResizeEdge edge)
{
    auto workspace = metadata->get_workspace();
    workspace->handle_request_resize(metadata, input_event, edge);
}

void Output::handle_modify_window(
    const std::shared_ptr<miracle::Container>& metadata,
    const miral::WindowSpecification& modifications)
{
    auto workspace = metadata->get_workspace();
    workspace->handle_modify_window(metadata, modifications);
}

void Output::handle_raise_window(const std::shared_ptr<miracle::Container>& metadata)
{
    auto workspace = metadata->get_workspace();
    workspace->handle_raise_window(metadata);
}

mir::geometry::Rectangle
Output::confirm_placement_on_display(
    const std::shared_ptr<miracle::Container>& metadata,
    MirWindowState new_state,
    const mir::geometry::Rectangle& new_placement)
{
    auto workspace = metadata->get_workspace();
    return workspace->confirm_placement_on_display(
        metadata, new_state, new_placement);
}

bool Output::select_window_from_point(int x, int y) const
{
    return get_active_workspace()->select_window_from_point(x, y);
}

void Output::select_window(miral::Window const& window)
{
    window_controller.select_active_window(window);
}

void Output::advise_new_workspace(int workspace)
{
    // Workspaces are always kept in sorted order
    auto new_workspace = std::make_shared<Workspace>(
        this, tools, workspace, config, window_controller, state, floating_window_manager);
    insert_sorted(workspaces, new_workspace, [](std::shared_ptr<Workspace> const& a, std::shared_ptr<Workspace> const& b)
    {
        return a->get_workspace() < b->get_workspace();
    });
}

void Output::advise_workspace_deleted(int workspace)
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

bool Output::advise_workspace_active(int key)
{
    std::shared_ptr<Workspace> from = nullptr;
    std::shared_ptr<Workspace> to = nullptr;
    for (auto const& workspace : workspaces)
    {
        if (workspace->get_workspace() == active_workspace)
            from = workspace;

        if (workspace->get_workspace() == key)
        {
            if (active_workspace == key)
                return true;

            to = workspace;
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

void Output::advise_application_zone_create(miral::Zone const& application_zone)
{
    if (application_zone.extents().contains(area))
    {
        application_zone_list.push_back(application_zone);
        for (auto& workspace : workspaces)
            workspace->recalculate_area();
    }
}

void Output::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto& zone : application_zone_list)
        if (zone == original)
        {
            zone = updated;
            for (auto& workspace : workspaces)
                workspace->recalculate_area();
            break;
        }
}

void Output::advise_application_zone_delete(miral::Zone const& application_zone)
{
    auto const original_size = application_zone_list.size();
    application_zone_list.erase(
        std::remove(application_zone_list.begin(), application_zone_list.end(), application_zone),
        application_zone_list.end());

    if (application_zone_list.size() != original_size)
    {
        for (auto& workspace : workspaces)
            workspace->recalculate_area();
    }
}

bool Output::point_is_in_output(int x, int y)
{
    return area.contains(geom::Point(x, y));
}

void Output::close_active_window()
{
    window_controller.close(state.active_window);
}

bool Output::resize_active_window(miracle::Direction direction) const
{
    return get_active_workspace()->resize_active_window(direction);
}

bool Output::select(miracle::Direction direction) const
{
    return get_active_workspace()->select(direction);
}

bool Output::move_active_window(miracle::Direction direction)
{
    return get_active_workspace()->move_active_window(direction);
}

bool Output::move_active_window_by_amount(Direction direction, int pixels)
{
    return get_active_workspace()->move_active_window_by_amount(direction, pixels);
}

bool Output::move_active_window_to(int x, int y)
{
    return get_active_workspace()->move_active_window_to(x, y);
}

void Output::request_vertical_layout() const
{
    get_active_workspace()->request_vertical_layout();
}

void Output::request_horizontal_layout() const
{
    get_active_workspace()->request_horizontal_layout();
}

void Output::toggle_layout() const
{
    get_active_workspace()->toggle_layout();
}

bool Output::toggle_fullscreen() const
{
    return get_active_workspace()->try_toggle_active_fullscreen();
}

void Output::toggle_pinned_to_workspace()
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
    {
        mir::log_error("toggle_pinned_to_workspace: metadata not found");
        return;
    }

    if (auto floating = Container::as_floating(metadata))
        floating->pinned(!floating->pinned());
}

void Output::set_is_pinned(bool is_pinned)
{
    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
    {
        mir::log_error("set_is_pinned: metadata not found");
        return;
    }

    if (auto floating = Container::as_floating(metadata))
        floating->pinned(is_pinned);
}

void Output::update_area(geom::Rectangle const& new_area)
{
    area = new_area;
    for (auto& workspace : workspaces)
        workspace->set_area(area);
}

std::vector<miral::Window> Output::collect_all_windows() const
{
    std::vector<miral::Window> windows;
    for (auto& workspace : get_workspaces())
    {
        workspace->for_each_window([&](std::shared_ptr<Container> const& container)
        {
            if (auto window = container->window())
                windows.push_back(*window);
        });
    }

    return windows;
}

void Output::request_toggle_active_float()
{
    if (tools.active_window() == Window())
    {
        mir::log_warning("request_toggle_active_float: active window unset");
        return;
    }

    auto metadata = window_controller.get_metadata(state.active_window);
    if (!metadata)
    {
        mir::log_error("request_toggle_active_float: metadata not found");
        return;
    }

    metadata->get_workspace()->toggle_floating(metadata);
}

void Output::add_immediately(miral::Window& window, ContainerType hint)
{
    auto& prev_info = window_controller.info_for(window);
    WindowSpecification spec = window_helpers::copy_from(prev_info);

    // If we are adding a window immediately, let's force it back into existence
    if (spec.state() == mir_window_state_hidden)
        spec.state() = mir_window_state_restored;

    ContainerType type = allocate_position(tools.info_for(window.application()), spec, hint);
    tools.modify_window(window, spec);
    auto metadata = advise_new_window(window_controller.info_for(window), type);
    handle_window_ready(window_controller.info_for(window), metadata);
}

geom::Rectangle Output::get_workspace_rectangle(int workspace) const
{
    // TODO: Support vertical workspaces one day in the future
    size_t x = (Workspace::workspace_to_number(workspace)) * area.size.width.as_int();
    return geom::Rectangle {
        geom::Point { geom::X { x },            geom::Y { 0 }             },
        geom::Size { area.size.width.as_int(), area.size.height.as_int() }
    };
}

glm::mat4 Output::get_transform() const
{
    return final_transform;
}

void Output::set_transform(glm::mat4 const& in)
{
    transform = in;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

void Output::set_position(glm::vec2 const& v)
{
    position_offset = v;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

glm::vec2 const& Output::get_position() const
{
    return position_offset;
}
