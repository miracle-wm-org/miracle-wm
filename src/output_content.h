#ifndef MIRACLE_SCREEN_H
#define MIRACLE_SCREEN_H

#include "tree.h"
#include "workspace_content.h"
#include <memory>
#include <miral/output.h>

namespace miracle
{
class WorkspaceManager;
class MiracleConfig;

class OutputContent
{
public:
    OutputContent(
        miral::Output const& output,
        WorkspaceManager& workspace_manager,
        geom::Rectangle const& area,
        miral::WindowManagerTools const& tools,
        std::shared_ptr<MiracleConfig> const& options);
    ~OutputContent() = default;

    std::shared_ptr<Tree> get_active_tree();
    [[nodiscard]] int get_active_workspace() const { return active_workspace; }
    WindowType allocate_position(miral::WindowSpecification& requested_specification);
    void advise_new_workspace(int workspace);
    void advise_workspace_deleted(int workspace);
    bool advise_workspace_active(int workspace);
    std::vector<std::shared_ptr<WorkspaceContent>> const& get_workspaces() { return workspaces; }
    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);
    bool point_is_in_output(int x, int y);

    geom::Rectangle const& get_area() { return area; }
    std::vector<miral::Zone> const& get_app_zones() { return application_zone_list; }
    miral::Output const& get_output() { return output; }
    [[nodiscard]] bool is_active() const { return is_active_; }
    void set_is_active(bool new_is_active) { is_active_ = new_is_active; }

private:
    miral::Output output;
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    geom::Rectangle area;
    std::shared_ptr<MiracleConfig> config;
    int active_workspace = -1;
    std::vector<std::shared_ptr<WorkspaceContent>> workspaces;
    std::vector<miral::Zone> application_zone_list;
    bool is_active_ = false;
};
    
}

#endif
