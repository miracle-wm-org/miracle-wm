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

#include "abstract_workspace.h"
#include "animator.h"

#include <memory>
#include <mir/synchronised.h>

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

class Workspace : public AbstractWorkspace
{
public:
    Workspace(
        std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
        std::shared_ptr<AbstractOutput> const& output,
        uint32_t id,
        std::optional<int> num,
        std::optional<std::string> name,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<PluginManager> const& plugin_manager);
    ~Workspace() override;

    void recalculate_area() override;

    void delete_container(std::shared_ptr<Container> const& container) override;
    bool move_container(Direction direction, Container&) override;
    bool add_to_root(Container& to_move) override;
    void show(mir::geometry::Point const& origin) override;
    void hide(mir::geometry::Point const& end) override;
    bool for_each_window(std::function<bool(std::shared_ptr<WindowContainer>)> const&) const override;
    void advise_focus_gained(std::shared_ptr<Container> const& container) override;
    [[nodiscard]] std::shared_ptr<AbstractOutput> get_output() const override;
    void set_output(std::shared_ptr<AbstractOutput> const&) override;
    [[nodiscard]] bool is_empty() const override;
    void graft(std::shared_ptr<Container> const&) override;
    void on_animation_end(bool is_hiding);
    [[nodiscard]] uint32_t id() const override { return id_; }
    [[nodiscard]] std::optional<int> num() const override { return sync.lock()->num_; }
    [[nodiscard]] std::optional<std::string> const& name() const override { return sync.lock()->name_; }
    void num(std::optional<int> n) override;
    void name(std::optional<std::string> const&) override;
    [[nodiscard]] mir::geometry::Rectangle area() const override;
    [[nodiscard]] std::optional<Gaps> outer_gaps() const override;
    void outer_gaps(std::optional<Gaps> const& gaps) override;
    [[nodiscard]] std::optional<Gaps> inner_gaps() const override;
    void inner_gaps(std::optional<Gaps> const& gaps) override;
    void transform(glm::mat4 const&) override;
    glm::mat4 transform() const override;
    void alpha(float) override;
    float alpha() const override;
    [[nodiscard]] nlohmann::json get_workspaces_json(bool is_output_focused) const override;
    [[nodiscard]] nlohmann::json to_json(bool is_output_focused) const override;
    [[nodiscard]] std::string display_name() const override;
    [[nodiscard]] std::shared_ptr<ParentContainer> get_root() const override;
    ParentContainer* get_layout_container() const override;
    void add_other_container(std::shared_ptr<Container> const& container, bool is_active) override;
    void remove_other_container(std::shared_ptr<Container> const& container) override;

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

    void for_each_container(std::function<void(std::shared_ptr<Container> const&)> const&);
    std::shared_ptr<ParentContainer> root() const;
    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::weak_ptr<AbstractOutput> output;
    uint32_t id_;
    mutable std::shared_ptr<ParentContainer> root_;
    std::vector<std::weak_ptr<Container>> other_containers;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<WorkspaceObserverRegistrar> registry;
    std::shared_ptr<Config> config;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<PluginManager> plugin_manager;
    AnimationHandle animation_handle;

    struct State
    {
        std::optional<int> num_;
        std::optional<std::string> name_;
        bool is_showing = false;
        std::weak_ptr<Container> last_selected_container;
        std::optional<Gaps> workspace_outer_gaps;
        std::optional<Gaps> workspace_inner_gaps;
        glm::mat4 transform_ = glm::mat4(1.f);
        float alpha_ = 1.f;
    };

    mir::Synchronised<State> sync;

    void on_animation_start(bool is_hiding);

    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult handle_move(Container& from, Direction direction);
};

} // miracle

#endif // MIRACLEWM_WORKSPACE_CONTENT_H
