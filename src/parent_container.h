/**
Copyright (C) 2024  Matthew Kosarek

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

#include "container.h"
#include "layout_scheme.h"
#include "mir/geometry/forward.h"
#include "miral/window_specification.h"
#include "window_controller.h"
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class LeafContainer;
class Config;
class CompositorState;
class OutputManager;

/// A parent container used to define the layout of containers beneath it.
class ParentContainer : public Container
{
public:
    ParentContainer(
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<Config> const& config,
        geom::Rectangle area,
        std::shared_ptr<WorkspaceInterface> const& workspace,
        std::shared_ptr<ParentContainer> const& parent,
        bool is_anchored);
    ~ParentContainer() override = default;
    virtual geom::Rectangle get_area() const;
    geom::Rectangle get_logical_area() const override;
    geom::Rectangle get_visible_area() const override;
    virtual size_t num_nodes() const;
    miral::WindowSpecification place_new_window(
        miral::WindowSpecification const& requested_specification);
    std::shared_ptr<LeafContainer> create_space_for_window(std::optional<size_t> index);
    std::shared_ptr<LeafContainer> confirm_window(miral::Window const&);
    void graft_existing(std::shared_ptr<Container> const& node, int index);
    std::shared_ptr<ParentContainer> convert_to_parent(std::shared_ptr<Container> const& container);
    void set_logical_area(geom::Rectangle const& target_rect, bool with_animations = true) override;

    /// Swaps two containers within this parent. Both containers MUST have this parent as their direct
    /// ancestor or else nothing happens.
    void swap_within_container(std::shared_ptr<Container> const& first, std::shared_ptr<Container> const& second);
    void remove(std::shared_ptr<Container> const& node);
    void commit_changes() override;
    std::shared_ptr<Container> at(size_t i) const;
    std::shared_ptr<LeafContainer> get_nth_window(size_t i) const;
    std::shared_ptr<Container> find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const;
    LayoutScheme get_direction() { return scheme; }
    std::vector<std::shared_ptr<Container>> const& get_sub_nodes() const;
    [[nodiscard]] std::optional<size_t> get_index_of_node(Container const* node) const;
    [[nodiscard]] std::optional<size_t> get_index_of_node(std::shared_ptr<Container> const& node) const;
    [[nodiscard]] std::optional<size_t> get_index_of_node(Container const&) const;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void set_parent(std::shared_ptr<ParentContainer> const&) override;
    void handle_ready() override;
    void handle_modify(miral::WindowSpecification const& specification) override;
    void handle_request_move(MirInputEvent const* input_event) override;
    void handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge) override;
    void handle_raise() override;
    bool resize(Direction direction, int pixels) override;
    bool set_size(std::optional<int> const& width, std::optional<int> const& height) override;
    bool toggle_fullscreen() override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout(bool cycle_thru_all) override;
    void on_focus_gained() override;
    void on_focus_lost() override;
    void on_move_to(mir::geometry::Point const& top_left) override;
    void on_resize(mir::geometry::Size const&) override;
    mir::geometry::Rectangle
    confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle) override;
    ContainerType get_type() const override;
    void show() override;
    void hide() override;
    void on_open() override;
    std::shared_ptr<WorkspaceInterface> get_workspace() const override;
    void set_workspace(std::shared_ptr<WorkspaceInterface> const& workspace) override;
    std::shared_ptr<OutputInterface> get_output() const override;
    glm::mat4 get_transform() const override;
    void set_transform(glm::mat4 transform) override;
    void set_workspace_transform(glm::mat4 const&) override;
    void set_workspace_alpha(float a) override;
    glm::mat4 get_workspace_transform() const override;
    glm::mat4 get_output_transform() const override;
    void set_alpha(float const alpha) override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t uint_32) override;
    bool is_focused() const override;
    std::optional<miral::Window> window() const override;
    bool select_next(Direction) override;
    bool pinned(bool) override;
    bool pinned() const override;
    bool move(Direction direction) override;
    bool move_by(Direction direction, int pixels) override;
    bool move_by(float dx, float dy) override;
    bool move_to(int x, int y, bool with_animations) override;
    bool move_to(Container& other) override;
    bool is_fullscreen() const override;
    bool toggle_tabbing() override;
    bool toggle_stacking() override;
    bool drag_start() override { return false; }
    void drag(int, int) override { }
    bool drag_stop() override { return false; }
    bool set_layout(LayoutScheme scheme) override;
    bool set_anchored(bool anchor);
    bool anchored() const override;
    ScratchpadState scratchpad_state() const override;
    void scratchpad_state(ScratchpadState) override;
    LayoutScheme get_layout() const override;
    bool matches(ContainerScope const&) const override;
    nlohmann::json to_json(bool is_workspace_visible) const override;
    [[nodiscard]] LayoutScheme get_scheme() const { return scheme; }
    static void swap(
        std::shared_ptr<ParentContainer> const& first_parent,
        size_t first_index,
        std::shared_ptr<ParentContainer> const& second_parent,
        size_t second_index);

private:
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<Config> config;
    geom::Rectangle logical_area;
    std::weak_ptr<WorkspaceInterface> workspace;
    std::weak_ptr<ParentContainer> parent;
    LayoutScheme scheme = LayoutScheme::horizontal;
    bool is_anchored;
    bool pinned_ = false;
    ScratchpadState scratchpad_state_ = ScratchpadState::none;
    bool is_shown = false;

    std::vector<std::shared_ptr<Container>> container_list;
    std::shared_ptr<LeafContainer> pending_node;

    geom::Rectangle create_space(std::optional<size_t> index);
    void relayout();
};

} // miracle

#endif // MIRACLEWM_PARENT_NODE_H
