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
        WindowTree(area, tools, options)
    });
    make_workspace_active(workspace);
}

bool Screen::make_workspace_active(char key)
{
    for (auto& workspace : workspaces)
    {
        if (workspace.workspace == key)
        {
            // Deactivate current workspace
            for (auto& other : workspaces)
            {
                if (other.workspace == active_workspace)
                {
                    hide(other);
                    break;
                }
            }

            // Active new workspace
            show(workspace);

            active_workspace = key;
            return true;
        }
    }

    active_workspace = key;
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
