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

#ifndef MIRACLE_WM_CONTAINER_GROUP_CONTAINER_H
#define MIRACLE_WM_CONTAINER_GROUP_CONTAINER_H

#include "container.h"
#include <vector>
#include <memory>

namespace miracle
{
class CompositorState;

/// A [Container] that contains [Container]s. This is often
/// used in a temporary way when mulitple [Container]s are selected
/// at once. The [ContainerGroupContainer] is incapable of performing
/// some actions by design. It weakly owns its members, meaning that
/// [Container]s may be removed from underneath it.
class ContainerGroupContainer : public Container
{
public:
    ContainerGroupContainer(CompositorState&);
    void add(std::shared_ptr<Container> const&);
    void remove(std::shared_ptr<Container> const&);

    ContainerType get_type() const override;
    void restore_state(MirWindowState state) override;
    std::optional<MirWindowState> restore_state() override;
    void commit_changes() override;
    mir::geometry::Rectangle get_logical_area() const override;
    void set_logical_area(mir::geometry::Rectangle const &rectangle) override;
    mir::geometry::Rectangle get_visible_area() const override;
    void constrain() override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void set_parent(std::shared_ptr<ParentContainer> const &ptr) override;
    size_t get_min_height() const override;
    size_t get_min_width() const override;
    void handle_ready() override;
    void handle_modify(miral::WindowSpecification const &specification) override;
    void handle_request_move(MirInputEvent const *input_event) override;
    void handle_request_resize(MirInputEvent const *input_event, MirResizeEdge edge) override;
    void handle_raise() override;
    bool resize(Direction direction) override;
    bool toggle_fullscreen() override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout() override;
    void on_open() override;
    void on_focus_gained() override;
    void on_focus_lost() override;
    void on_move_to(mir::geometry::Point const &top_left) override;
    mir::geometry::Rectangle
        confirm_placement(MirWindowState state, mir::geometry::Rectangle const &rectangle) override;
    Workspace *get_workspace() const override;
    Output *get_output() const override;
    glm::mat4 get_transform() const override;
    void set_transform(glm::mat4 transform) override;
    glm::mat4 get_workspace_transform() const override;
    glm::mat4 get_output_transform() const override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t uint_32) override;
    bool is_focused() const override;
    bool is_fullscreen() const override;
    std::optional<miral::Window> window() const override;
    bool select_next(Direction direction) override;
    bool pinned() const override;
    bool pinned(bool b) override;
    bool move(Direction direction) override;
    bool move_by(Direction direction, int pixels) override;
    bool move_to(int x, int y) override;

private:
    std::vector<std::weak_ptr<Container>> containers;
    CompositorState& state;
};

} // miracle

#endif //MIRACLE_WM_CONTAINER_GROUP_CONTAINER_H
