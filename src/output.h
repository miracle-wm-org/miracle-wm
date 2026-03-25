/**
Copyright (C) 2025  Matthew Kosarek

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

#ifndef MIRACLE_WM_OUTPUT_H
#define MIRACLE_WM_OUTPUT_H

#include "abstract_output.h"
#include "display_config.h"
#include "synchronized_recursive.h"

namespace miracle
{
class PluginManager;
class ShellApplicationManager;

class Output final : public AbstractOutput, public std::enable_shared_from_this<Output>
{
public:
    Output(
        std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
        std::string name,
        int id,
        geom::Rectangle const& area,
        OutputConfigDetails const& output_config,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<Config> const& options,
        std::shared_ptr<WindowController> const&,
        std::shared_ptr<Animator> const&,
        std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
        std::shared_ptr<PluginManager> const& plugin_manager);
    ~Output() override;

    std::shared_ptr<WindowContainer> intersect(float x, float y) override;
    std::shared_ptr<WindowContainer> intersect_leaf(float x, float y, bool ignore_selected) override;
    void delete_container(std::shared_ptr<Container> const& container) override;
    void advise_new_workspace(WorkspaceCreationData const&&) override;
    void advise_workspace_deleted(WorkspaceManager& workspace_manager, uint32_t id) override;
    bool advise_workspace_active(WorkspaceManager& workspace_manager, uint32_t id) override;
    void advise_application_zone_create(miral::Zone const& application_zone) override;
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override;
    void advise_application_zone_delete(miral::Zone const& application_zone) override;
    void move_workspace_to(WorkspaceManager& workspace_manager, AbstractWorkspace* workspace) override;
    bool point_is_in_output(int x, int y) override;
    void update_area(geom::Rectangle const& area) override;
    void graft(std::shared_ptr<Container> const& container) override;
    void set_transform(glm::mat4 const& in) override;
    void set_info(int id, std::string name) override;
    void set_defunct() override;
    void unset_defunct() override;

    [[nodiscard]] std::shared_ptr<AbstractWorkspace> active() const override;
    [[nodiscard]] std::vector<std::shared_ptr<AbstractWorkspace>> get_workspaces() const override { return sync.lock()->workspaces; }
    [[nodiscard]] geom::Rectangle const& get_area() const override { return sync.lock()->area; }
    [[nodiscard]] std::vector<miral::Zone> const& get_app_zones() const override { return application_zone_list; }
    [[nodiscard]] std::string const& name() const override { return sync.lock()->name_; }
    [[nodiscard]] bool is_defunct() const override { return sync.lock()->is_defunct_; }
    [[nodiscard]] int id() const override { return sync.lock()->id_; }
    [[nodiscard]] glm::mat4 get_transform() const override;
    [[nodiscard]] AbstractWorkspace const* workspace(uint32_t id) const override;
    [[nodiscard]] nlohmann::json to_json(bool is_focused) const override;
    [[nodiscard]] nlohmann::json get_outputs_json(bool is_focused) const override;
    [[nodiscard]] bool is_primary() const override;

private:
    void insert_workspace_sorted(std::shared_ptr<AbstractWorkspace> const& new_workspace);

    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    OutputConfigDetails const output_config;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<Config> config;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::vector<miral::Zone> application_zone_list;
    std::shared_ptr<PluginManager> plugin_manager;

    struct State
    {
        int id_;
        std::string name_;
        geom::Rectangle area;

        /// The transform applied to the entire output.
        glm::mat4 transform = glm::mat4(1.f);

        bool is_defunct_ = false;

        std::weak_ptr<AbstractWorkspace> active_workspace;

        std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    };

    SynchronisedRecursive<State> sync;
};
}

#endif // MIRACLE_WM_OUTPUT_H
