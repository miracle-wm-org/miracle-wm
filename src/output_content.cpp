#define MIR_LOG_COMPONENT "output_content"

#include "output_content.h"
#include "window_helpers.h"
#include "workspace_manager.h"
#include <miral/window_info.h>
#include <mir/log.h>

using namespace miracle;

OutputContent::OutputContent(
    miral::Output const& output,
    WorkspaceManager& workspace_manager,
    geom::Rectangle const& area,
    miral::WindowManagerTools const& tools,
    std::shared_ptr<MiracleConfig> const& config)
    : output{output},
      workspace_manager{workspace_manager},
      area{area},
      tools{tools},
      config{config}
{
}

std::shared_ptr<Tree> OutputContent::get_active_tree() const
{
    return get_active_workspace()->get_tree();
}

std::shared_ptr<WorkspaceContent> OutputContent::get_active_workspace() const
{
    for (auto& info : workspaces)
    {
        if (info->get_workspace() == active_workspace)
            return info;
    }

    throw std::runtime_error("get_active_workspace: unable to find the active workspace. We shouldn't be here!");
    return nullptr;
}

WindowType OutputContent::allocate_position(miral::WindowSpecification& requested_specification)
{
    if (!window_helpers::is_tileable(requested_specification))
        return WindowType::other;

    requested_specification = get_active_tree()->allocate_position(requested_specification);
    return WindowType::tiled;
}

void OutputContent::advise_new_window(miral::WindowInfo const& window_info, WindowType type)
{
    std::shared_ptr<WindowMetadata> metadata = nullptr;
    switch (type)
    {
        case WindowType::tiled:
        {
            auto node = get_active_tree()->advise_new_window(window_info);
            metadata = std::make_shared<WindowMetadata>(WindowType::tiled, window_info.window(), this);
            metadata->associate_to_node(node);
            break;
        }
        case WindowType::other:
            if (window_info.state() == MirWindowState::mir_window_state_attached)
            {
                tools.select_active_window(window_info.window());
            }
            metadata = std::make_shared<WindowMetadata>(WindowType::other, window_info.window(), this);
            break;
        default:
            mir::log_error("Unsupported window type: %d", (int)type);
            break;
    }

    if (metadata)
    {
        miral::WindowSpecification spec;
        spec.userdata() = metadata;
        tools.modify_window(window_info.window(), spec);
    }
    else
    {
        mir::log_error("Window failed to set metadata");
    }
}

void OutputContent::handle_window_ready(miral::WindowInfo &window_info, std::shared_ptr<miracle::WindowMetadata> const& metadata)
{
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

void OutputContent::advise_focus_gained(const std::shared_ptr<miracle::WindowMetadata> &metadata)
{
    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->advise_focus_gained(metadata->get_window());
            tools.raise_tree(metadata->get_window());
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void OutputContent::advise_focus_lost(const std::shared_ptr<miracle::WindowMetadata> &metadata)
{
    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->advise_focus_lost(metadata->get_window());
            tools.raise_tree(metadata->get_window());
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void OutputContent::advise_delete_window(const std::shared_ptr<miracle::WindowMetadata> &metadata)
{
    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            metadata->get_tiling_node()->get_tree()->advise_delete_window(metadata->get_window());
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void OutputContent::advise_state_change(const std::shared_ptr<miracle::WindowMetadata> &metadata, MirWindowState state)
{
    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            if (get_active_tree().get() != metadata->get_tiling_node()->get_tree())
                break;

            metadata->get_tiling_node()->get_tree()->advise_state_change(metadata->get_window(), state);
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

void OutputContent::handle_modify_window(const std::shared_ptr<miracle::WindowMetadata> &metadata,
                                         const miral::WindowSpecification &modifications)
{
    switch (metadata->get_type())
    {
        case WindowType::tiled:
        {
            if (get_active_tree().get() != metadata->get_tiling_node()->get_tree())
                break;

            if (modifications.state().is_set())
            {
                if (modifications.state().value() == mir_window_state_fullscreen || modifications.state().value() == mir_window_state_maximized)
                    metadata->get_tiling_node()->get_tree()->advise_fullscreen_window(metadata->get_window());
                else if (modifications.state().value() == mir_window_state_restored)
                    metadata->get_tiling_node()->get_tree()->advise_restored_window(metadata->get_window());
            }

            metadata->get_tiling_node()->get_tree()->constrain(metadata->get_window());
            tools.modify_window(metadata->get_window(), modifications);
            break;
        }
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            return;
    }
}

mir::geometry::Rectangle
OutputContent::confirm_placement_on_display(
    const std::shared_ptr<miracle::WindowMetadata> &metadata,
    MirWindowState new_state,
    const mir::geometry::Rectangle &new_placement)
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
        default:
            mir::log_error("Unsupported window type: %d", (int)metadata->get_type());
            break;
    }
    return new_placement;
}

void OutputContent::select_window_from_point(int x, int y)
{
    get_active_tree()->select_window_from_point(x, y);
}

void OutputContent::advise_new_workspace(int workspace)
{
    workspaces.push_back(
        std::make_shared<WorkspaceContent>(this, tools, workspace, config));
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
    for (auto& workspace : workspaces)
    {
        if (workspace->get_workspace() == key)
        {
            std::shared_ptr<WorkspaceContent> previous_workspace = nullptr;
            for (auto& other : workspaces)
            {
                if (other->get_workspace() == active_workspace)
                {
                    previous_workspace = other;
                    other->hide();
                    break;
                }
            }

            active_workspace = key;
            workspace->show();

            // Important: Delete the workspace only after we have shown the new one because we may want
            // to move a node to the new workspace.
            if (previous_workspace != nullptr)
            {
                auto active_tree = previous_workspace->get_tree();
                if (active_tree->is_empty())
                    workspace_manager.delete_workspace(previous_workspace->get_workspace());
            }
            return true;
        }
    }

    return false;
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
    get_active_tree()->close_active_window();
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
    return get_active_tree()->try_move_active_window(direction);
}

void OutputContent::request_vertical()
{
    get_active_tree()->request_vertical();
}

void OutputContent::request_horizontal()
{
    get_active_tree()->request_horizontal();
}

void OutputContent::toggle_resize_mode()
{
    get_active_tree()->toggle_resize_mode();
}

void OutputContent::toggle_fullscreen()
{
    get_active_tree()->try_toggle_active_fullscreen();
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
        workspace->get_tree()->foreach_node([&](auto node)
        {
            if (node->is_window())
            {
                windows.push_back(node->get_window());
            }
        });
    }
    return windows;
}