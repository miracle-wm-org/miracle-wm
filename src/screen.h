#ifndef MIRACLE_SCREEN_H
#define MIRACLE_SCREEN_H

#include "tree.h"
#include <memory>
#include <miral/output.h>

namespace miracle
{

struct WorkspaceManager;
class MiracleConfig;

struct NodeResurrection
{
    std::shared_ptr<Node> node;
    MirWindowState state;
};

struct ScreenWorkspaceInfo
{
    int workspace;
    std::shared_ptr<Tree> tree;
    std::vector<NodeResurrection> nodes_to_resurrect;
};

/// A screen is comprised of a map of workspaces, each having their own tree.
// Workspaces are shared across screens such that screens a workspace with a
// particular index can ONLY ever live on one screen at a time.
class Screen
{
public:
    Screen(
        miral::Output const& output,
        WorkspaceManager& workspace_manager,
        geom::Rectangle const& area,
        miral::WindowManagerTools const& tools,
        std::shared_ptr<MiracleConfig> const& options);
    ~Screen() = default;

    Tree& get_active_tree();
    int get_active_workspace() const { return active_workspace; }
    void advise_new_workspace(int workspace);
    void advise_workspace_deleted(int workspace);
    bool advise_workspace_active(int workspace);
    std::vector<ScreenWorkspaceInfo>& get_workspaces() { return workspaces; }
    ScreenWorkspaceInfo const& get_workspace(int key);
    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);
    bool point_is_in_output(int x, int y);

    geom::Rectangle const& get_area() { return area; }
    std::vector<miral::Zone> const& get_app_zones() { return application_zone_list; }
    miral::Output const& get_output() { return output; }
    bool is_active() const { return is_active_; }
    void set_is_active(bool new_is_active) { is_active_ = new_is_active; }

private:
    miral::Output output;
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    geom::Rectangle area;
    std::shared_ptr<MiracleConfig> config;
    int active_workspace = -1;
    std::vector<ScreenWorkspaceInfo> workspaces;
    std::vector<miral::Zone> application_zone_list;
    bool is_active_ = false;

    void hide(ScreenWorkspaceInfo&);
    void show(ScreenWorkspaceInfo&);
};
    
}

#endif
