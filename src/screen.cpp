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
    workspace_manager.request_first_available_workspace(this);
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
    active_workspace = workspace;
    workspaces.push_back({
        workspace,
        WindowTree(area, tools, options)
    });
}

bool Screen::make_workspace_active(char key)
{
    for (auto workspace : workspaces)
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

    return false;
}

void Screen::hide(ScreenWorkspaceInfo& info)
{
    info.tree.foreach_node([&](auto node)
    {
        if (node->is_window())
        {
            miral::WindowInfo& window_info = tools.info_for(node->get_window());
            WindowSpecification modifications;

            info.nodes_to_resurrect.push_back({
                node,
                window_info.state()
            });

            modifications.state() = mir_window_state_hidden;
            tools.place_and_size_for_state(modifications, window_info);
            tools.modify_window(window_info.window(), modifications);
        }
    });
}

void Screen::show(ScreenWorkspaceInfo& info)
{
    info.tree.foreach_node([&](auto node)
    {
         if (node->is_window())
         {
             auto& window_info = tools.info_for(node->get_window());
             WindowSpecification modifications;

             for (auto other_node : info.nodes_to_resurrect)
             {
                 if (other_node.node == node)
                 {
                     modifications.state() = other_node.state;
                     tools.place_and_size_for_state(modifications, window_info);
                     tools.modify_window(window_info.window(), modifications);
                 }
             }
         }
    });
    info.nodes_to_resurrect.clear();
}