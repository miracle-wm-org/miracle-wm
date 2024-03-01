#include "output_content.h"
#include "window_helpers.h"
#include "workspace_manager.h"
#include <miral/window_info.h>

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

std::shared_ptr<Tree> OutputContent::get_active_tree()
{
    for (auto& info : workspaces)
    {
        if (info->get_workspace() == active_workspace)
            return info->get_tree();
    }

    throw std::runtime_error("Unable to find the active tree. We shouldn't be here");
    return nullptr;
}

WindowType OutputContent::allocate_position(miral::WindowSpecification& requested_specification)
{
    if (!window_helpers::is_tileable(requested_specification))
        return WindowType::other;

    requested_specification = get_active_tree()->allocate_position(requested_specification);
    return WindowType::tiled;
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