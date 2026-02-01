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

#ifndef MIRACLEWM_WORKSPACE_CONTENT_H
#define MIRACLEWM_WORKSPACE_CONTENT_H

#include "animator.h"
#include "workspace_interface.h"

#include <memory>
#include <miral/window_manager_tools.h>

namespace miracle
{
class OutputManager;
class WindowController;
class Config;
class CompositorState;
class WorkspaceObserverRegistrar;
class Animator;
class PluginManager;
class ShellApplicationManager;

struct WorkspaceIdentifier
{
    std::optional<int> const number;
    std::optional<std::string> const name;
};

class Workspace : public WorkspaceInterface, public std::enable_shared_from_this<Workspace>
{
public:
    Workspace(
        std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
        std::shared_ptr<OutputInterface> const& output,
        uint32_t id,
        std::optional<int> num,
        std::optional<std::string> name,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
        std::shared_ptr<PluginManager> const& plugin_manager);
    ~Workspace() override;

    void set_area(mir::geometry::Rectangle const&) override;
    void recalculate_area() override;

    AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint const& hint) override;
    void delete_container(std::shared_ptr<Container> const& container) override;
    bool move_container(Direction direction, Container&) override;
    bool add_to_root(Container& to_move) override;
    void show(mir::geometry::Point const& origin) override;
    void hide(mir::geometry::Point const& end) override;
    void transfer_pinned_windows_to(std::shared_ptr<WorkspaceInterface> const& other) override;
    bool for_each_window(std::function<bool(std::shared_ptr<Container>)> const&) const override;
    std::shared_ptr<ParentContainer> create_floating_tree(mir::geometry::Rectangle const& area) override;
    void advise_focus_gained(std::shared_ptr<Container> const& container) override;
    [[nodiscard]] std::shared_ptr<OutputInterface> get_output() const override;
    void set_output(std::shared_ptr<OutputInterface> const&) override;
    [[nodiscard]] bool is_empty() const override;
    void graft(std::shared_ptr<Container> const&) override;
    void on_animation_end(bool is_hiding);
    [[nodiscard]] uint32_t id() const override { return id_; }
    [[nodiscard]] std::optional<int> num() const override { return num_; }
    [[nodiscard]] std::optional<std::string> const& name() const override { return name_; }
    void num(std::optional<int> n) override;
    void name(std::optional<std::string> const&) override;
    [[nodiscard]] std::optional<Gaps> outer_gaps() const override;
    void outer_gaps(std::optional<Gaps> const& gaps) override;
    [[nodiscard]] std::optional<Gaps> inner_gaps() const override;
    void inner_gaps(std::optional<Gaps> const& gaps) override;
    void transform(glm::mat4 const&) override;
    glm::mat4 transform() const override;
    void alpha(float) override;
    float alpha() const override;
    std::vector<std::shared_ptr<ParentContainer>> trees() const override;
    [[nodiscard]] nlohmann::json get_workspaces_json(bool is_output_focused) const override;
    [[nodiscard]] nlohmann::json to_json(bool is_output_focused) const override;
    [[nodiscard]] std::string display_name() const override;
    [[nodiscard]] std::shared_ptr<ParentContainer> get_root() const override;

private:
    struct MoveResult
    {
        enum
        {
            traversal_type_invalid,
            traversal_type_insert,
            traversal_type_prepend,
            traversal_type_append
        } traversal_type
            = traversal_type_invalid;
        std::shared_ptr<Container> node = nullptr;
    };

    std::shared_ptr<ParentContainer> root() const;

    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::weak_ptr<OutputInterface> output;
    uint32_t id_;
    std::optional<int> num_;
    std::optional<std::string> name_;
    mutable std::shared_ptr<ParentContainer> root_;
    std::vector<std::shared_ptr<ParentContainer>> floating_trees;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<WorkspaceObserverRegistrar> registry;
    std::shared_ptr<Config> config;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::shared_ptr<PluginManager> plugin_manager;
    AnimationHandle animation_handle;
    bool is_showing = false;
    std::weak_ptr<Container> last_selected_container;
    std::optional<Gaps> workspace_outer_gaps;
    std::optional<Gaps> workspace_inner_gaps;
    glm::mat4 transform_ = glm::mat4(1.f);
    float alpha_ = 1.f;

    void on_animation_start(bool is_hiding);

    /// Retrieves the container that is currently being used for layout
    std::shared_ptr<ParentContainer> get_layout_container();

    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult handle_move(Container& from, Direction direction);
};

} // miracle

#endif // MIRACLEWM_WORKSPACE_CONTENT_H
