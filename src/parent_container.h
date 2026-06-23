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

#ifndef MIRACLEWM_PARENT_NODE_H
#define MIRACLEWM_PARENT_NODE_H

#include "collection_container.h"
#include "layout_scheme.h"
#include "mir/geometry/forward.h"
#include "shell_application_manager.h"
#include "window_controller.h"
#include <mir/geometry/rectangle.h>
#include <miral/window_specification.h>

namespace geom = mir::geometry;

namespace miracle
{
class ParentBackgroundInternalClient;

class LeafContainer;
class Config;
class CompositorState;
class OutputManager;

/// A parent container used to define the layout of containers beneath it.
class ParentContainer : public CollectionContainer
{
public:
    ParentContainer(
        std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<Config> const& config,
        geom::Rectangle area,
        std::shared_ptr<AbstractWorkspace> const& workspace,
        std::shared_ptr<ParentContainer> const& parent);
    ~ParentContainer() override;

protected:
    /// For use by test mocks only — not for production construction.
    ParentContainer() :
        CollectionContainer(0)
    {
    }

public:
    // Container
    geom::Rectangle get_logical_area() const override;
    void set_logical_area(geom::Rectangle const& target_rect, bool with_animations) override;
    void commit_changes() override;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void set_parent(std::shared_ptr<ParentContainer> const&) override;
    void handle_raise() override;
    bool set_size(std::optional<int> const& width, std::optional<int> const& height) override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout(bool cycle_thru_all) override;
    void on_focus_gained() override;
    void show() override;
    void hide() override;
    std::shared_ptr<AbstractWorkspace> get_workspace() const override;
    void set_workspace(std::shared_ptr<AbstractWorkspace> const& workspace) override;
    std::shared_ptr<AbstractOutput> get_output() const override;
    glm::mat4 get_animation_transform() const override;
    void set_animation_transform(glm::mat4 transform) override;
    void set_workspace_transform(glm::mat4 const&) override;
    void set_workspace_alpha(float a) override;
    glm::mat4 get_workspace_transform() const override;
    glm::mat4 get_output_transform() const override;
    void set_animation_alpha(float const alpha) override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t uint_32) override;
    bool is_focused() const override;
    std::optional<miral::Window> window() const override;
    bool pinned(bool) override { return false; }
    bool pinned() const override { return false; }
    bool move_by(float dx, float dy) override;
    bool move_to(int x, int y, bool with_animations) override;
    bool toggle_tabbing() override;
    bool toggle_stacking() override;
    bool set_layout(LayoutScheme scheme) override;
    bool anchored() const override { return true; }
    ScratchpadState scratchpad_state() const override;
    void scratchpad_state(ScratchpadState) override;
    LayoutScheme get_layout() const override;
    nlohmann::json to_json(bool is_workspace_visible) const override;

    // CollectionContainer
    void remove_child(std::shared_ptr<Container> const& container) override;
    void add_child(std::shared_ptr<Container> const& container, size_t index) override;
    std::vector<std::shared_ptr<Container>> children() const override;

    // Parent
    miral::WindowSpecification place_new_window(
        miral::WindowSpecification const& requested_specification,
        std::optional<size_t> index);
    std::shared_ptr<Container> confirm_window(miral::Window const&);
    std::shared_ptr<ParentContainer> convert_to_parent(std::shared_ptr<Container> const& container);
    /// Swaps two containers within this parent. Both containers MUST have this parent as their direct
    /// ancestor or else nothing happens.
    void swap_within_container(std::shared_ptr<Container> const& first, std::shared_ptr<Container> const& second);
    std::shared_ptr<LeafContainer> get_nth_window(size_t i) const;
    std::shared_ptr<Container> find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const;
    [[nodiscard]] LayoutScheme get_scheme() const { return scheme_; }
    [[nodiscard]] std::optional<size_t> get_index_of_node(Container const* node) const;
    [[nodiscard]] std::optional<size_t> get_index_of_node(std::shared_ptr<Container> const& node) const;
    [[nodiscard]] std::optional<size_t> get_index_of_node(Container const&) const;

    static void swap(
        std::shared_ptr<ParentContainer> const& first_parent,
        size_t first_index,
        std::shared_ptr<ParentContainer> const& second_parent,
        size_t second_index);

private:
    class ParentContainerBackgroundPositioner : public ShellApplicationDelegate
    {
    public:
        explicit ParentContainerBackgroundPositioner(ParentContainer* parent);
        void place_window(miral::WindowSpecification& specification) override;
        void handle_ready(std::shared_ptr<Container> const& in) override;
        void set_area(mir::geometry::Rectangle const& area);

    private:
        ParentContainer* parent;
        std::weak_ptr<Container> container;
    };

    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<Config> config;
    std::vector<std::shared_ptr<Container>> container_list_;
    std::weak_ptr<ParentContainer> parent_;
    geom::Rectangle logical_area_;
    std::weak_ptr<AbstractWorkspace> workspace_;
    LayoutScheme scheme_ = LayoutScheme::horizontal;
    ScratchpadState scratchpad_state_ = ScratchpadState::none;
    bool is_shown_ = false;
    std::shared_ptr<LeafContainer> pending_node_;
    std::optional<ShellApplicationId> shell_application_id_;
    std::weak_ptr<ParentContainerBackgroundPositioner> shell_application_positioner_;

    geom::Rectangle create_space(std::optional<size_t> index);
    std::shared_ptr<LeafContainer> create_space_for_window(std::optional<size_t> index);
    void relayout();
    void raise_children();
    void update_background_client_area();
    void try_remove_background_client();
};

} // miracle

#endif // MIRACLEWM_PARENT_NODE_H
