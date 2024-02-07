#ifndef MIRACLE_SCREEN_H
#define MIRACLE_SCREEN_H

#include "window_tree.h"
#include <memory>

namespace miracle
{

struct WorkspaceManager;

struct NodeResurrection
{
    std::shared_ptr<Node> node;
    MirWindowState state;
};

struct ScreenWorkspaceInfo
{
    char workspace;
    WindowTree tree;
    std::vector<NodeResurrection> nodes_to_resurrect;
};

/// A screen is comprised of a map of workspaces, each having their own tree.
// Workspaces are shared across screens such that screens a workspace with a
// particular index can ONLY ever live on one screen at a time.
class Screen
{
public:
    Screen(
        WorkspaceManager& workspace_manager,
        geom::Rectangle const& area,
        miral::WindowManagerTools const& tools,
        WindowTreeOptions const& options);
   ~Screen() = default;

   WindowTree& get_active_tree();
   void advise_new_workspace(char workspace);
   bool make_workspace_active(char workspace);
   std::vector<ScreenWorkspaceInfo>& get_workspaces() { return workspaces; }

private:
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    geom::Rectangle area;
    WindowTreeOptions options;
    char active_workspace;
    std::vector<ScreenWorkspaceInfo> workspaces;

    void hide(ScreenWorkspaceInfo&);
    void show(ScreenWorkspaceInfo&);
};
    
}

#endif
