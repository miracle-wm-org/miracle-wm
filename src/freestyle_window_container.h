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

#ifndef MIRACLE_FREESTYLE_WINDOW_CONTAINER_H
#define MIRACLE_FREESTYLE_WINDOW_CONTAINER_H

#include "synchronized_recursive.h"
#include "window_container.h"
#include "window_controller.h"

namespace miracle
{
class CompositorState;
class Config;

/// A container for non-tiled application windows such as dialogs, satellites,
/// and utility windows. Unlike ShellComponentContainer, FreestyleWindowContainer
/// is workspace-aware, focusable, and supports move/resize operations. If the
/// window type warrants it (dialog or satellite), a border is rendered.
class FreestyleWindowContainer : public WindowContainer
{
public:
    FreestyleWindowContainer(
        miral::Window const& window,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<AbstractWorkspace> const& workspace,
        std::shared_ptr<Config> const& config,
        bool has_border);

    void show() override;
    void hide() override;
    geom::Rectangle get_logical_area() const override;
    void set_logical_area(geom::Rectangle const& area, bool with_animations) override;
    geom::Rectangle get_visible_area() const override;
    void constrain() override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void commit_changes() override;
    void set_parent(std::shared_ptr<ParentContainer> const&) override;
    size_t get_min_height() const override;
    size_t get_min_width() const override;
    void handle_ready() override;
    void handle_modify(miral::WindowSpecification const&, bool hidden) override;
    void handle_request_move(MirInputEvent const* input_event) override;
    void handle_raise() override;
    void on_focus_gained() override;
    bool resize(Direction direction, int pixels) override;
    bool set_size(std::optional<int> const& width, std::optional<int> const& height) override;
    bool toggle_fullscreen() override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout(bool cycle_thru_all) override;
    void on_move_to(geom::Point const& top_left) override;
    void on_resize(geom::Size const& size) override;
    mir::geometry::Rectangle confirm_placement(MirWindowState, mir::geometry::Rectangle const&) override;
    std::shared_ptr<AbstractWorkspace> get_workspace() const override;
    void set_workspace(std::shared_ptr<AbstractWorkspace> const&) override;
    std::shared_ptr<AbstractOutput> get_output() const override;
    glm::mat4 get_output_transform() const override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t) override;
    bool is_focused() const override;
    bool is_fullscreen() const override;
    bool is_maximized() const;
    bool needs_outline() const override;
    bool select_next(Direction) override;
    bool pinned() const override;
    bool pinned(bool) override;
    bool move(Direction) override;
    bool move_by(Direction direction, int pixels) override;
    bool move_to(Container& other) override;
    bool move_to(int x, int y, bool with_animations) override;
    bool move_by(float dx, float dy) override;
    bool toggle_tabbing() override;
    bool toggle_stacking() override;
    bool drag_start() override;
    void drag(int x, int y) override;
    bool drag_stop() override;
    bool set_layout(LayoutScheme scheme) override;
    bool anchored() const override;
    void scratchpad_state(ScratchpadState) override;
    ScratchpadState scratchpad_state() const override;
    const std::vector<std::string>& get_marks() const override;
    LayoutScheme get_layout() const override;
    bool matches(ContainerScope const&) const override;
    bool can_animate() override;
    nlohmann::json to_json(bool is_workspace_visible) const override;

private:
    struct State
    {
        std::weak_ptr<AbstractWorkspace> workspace_;
        std::optional<RestoreResult> restore_result;
        std::optional<MirWindowState> next_state;
        std::optional<geom::Rectangle> pre_fullscreen_area;
        std::optional<MirDepthLayer> next_depth_layer;
        uint32_t handle_ = 0;
        bool is_focused_ = false;
        bool is_dragging_ = false;
        bool pinned = false;
        geom::Point dragged_position;
    };

    void restore_from_maximize();

    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<Config> config;
    bool has_border_;
    SynchronisedRecursive<State> sync;
};
}

#endif
