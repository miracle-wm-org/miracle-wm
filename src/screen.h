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
    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);
    bool point_is_in_output(int x, int y);

    geom::Rectangle const& get_area() { return area; }
    std::vector<miral::Zone> const& get_app_zones() { return application_zone_list; }

private:
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    geom::Rectangle area;
    WindowTreeOptions options;
    char active_workspace;
    std::vector<ScreenWorkspaceInfo> workspaces;
    std::vector<miral::Zone> application_zone_list;

    void hide(ScreenWorkspaceInfo&);
    void show(ScreenWorkspaceInfo&);
};
    
}

#endif
