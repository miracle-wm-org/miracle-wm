#include "screen.h"
#include "workspace_manager.h"
#include <miral/window_info.h>

using namespace miracle;

Screen::Screen(
    WorkspaceManager& workspace_manager,
    geom::Rectangle const& area,
    miral::WindowManagerTools const& tools,
    WindowTreeOptions const& options)
    : workspace_manager{workspace_manager},
      area{area},
      tools{tools},
      options{options}
{
}

WindowTree &Screen::get_active_tree()
{
    for (auto& info : workspaces)
    {
        if (info.workspace == active_workspace)
            return info.tree;
    }

    throw std::runtime_error("Unable to find the active tree. We shouldn't be here");
}

void Screen::advise_new_workspace(char workspace)
{
    workspaces.push_back({
        workspace,
        WindowTree(this, tools, options)
    });
    make_workspace_active(workspace);
}

void Screen::advise_workspace_deleted(char workspace)
{
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (it->workspace == workspace)
        {
            workspaces.erase(it);
            return;
        }
    }
}

bool Screen::make_workspace_active(char key)
{
    for (auto& workspace : workspaces)
    {
        if (workspace.workspace == key)
        {
            ScreenWorkspaceInfo* previous_workspace = nullptr;
            for (auto& other : workspaces)
            {
                if (other.workspace == active_workspace)
                {
                    previous_workspace = &other;
                    hide(other);
                    break;
                }
            }

            active_workspace = key;
            show(workspace);

            // Important: Delete the workspace only after we have shown the new one because we may want
            // to move a node to the new workspace.
            if (previous_workspace != nullptr)
            {
                auto& active_tree = previous_workspace->tree;
                if (active_tree.is_empty())
                    workspace_manager.delete_workspace(previous_workspace->workspace);
            }
            return true;
        }
    }

    return false;
}

void Screen::hide(ScreenWorkspaceInfo& info)
{
    info.tree.hide();
}

void Screen::show(ScreenWorkspaceInfo& info)
{
     info.tree.show();
}

void Screen::advise_application_zone_create(miral::Zone const& application_zone)
{
    if (application_zone.extents().contains(area))
    {
        application_zone_list.push_back(application_zone);
        for (auto& workspace : workspaces)
            workspace.tree._recalculate_root_node_area();
    }
}

void Screen::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto& zone : application_zone_list)
        if (zone == original)
        {
            zone = updated;
            for (auto& workspace : workspaces)
                workspace.tree._recalculate_root_node_area();
            break;
        }
}

void Screen::advise_application_zone_delete(miral::Zone const& application_zone)
{
    if (std::remove(application_zone_list.begin(), application_zone_list.end(), application_zone) != application_zone_list.end())
    {
        for (auto& workspace : workspaces)
            workspace.tree._recalculate_root_node_area();
    }
}

bool Screen::point_is_in_output(int x, int y)
{
    return area.contains(geom::Point(x, y));
}