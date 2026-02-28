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

#ifndef MIRACLE_PLUGIN_MANAGED_CONTAINER
#define MIRACLE_PLUGIN_MANAGED_CONTAINER

#include "render_data_manager.h"
#include "window_container.h"

namespace miracle
{
class WindowController;
class CompositorState;

/// A container that is managed by a plugin.
///
/// The idea of this class is to allow plugins to create and manage
/// containers that can be used by Miracle in the same way as normal
/// containers. This would allow for more advanced plugin functionality,
/// such as custom tiling algorithms or container behaviors.
///
/// Unlike #ShellComponentContainer, a plugin managed container *can* be
/// associated with a specific workspace. However, it is *not* associated
/// with a tiling grid, like other containers.
class PluginManagedContainer : public WindowContainer
{
public:
    PluginManagedContainer(
        PluginHandle plugin_handle,
        miral::Window const& window,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state);
    ~PluginManagedContainer() override;

    void show() override;
    void hide() override;
    geom::Rectangle get_logical_area() const override;
    void set_logical_area(geom::Rectangle const& area, bool with_animations) override;
    geom::Rectangle get_visible_area() const override;
    void constrain() override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void commit_changes() override;
    void set_parent(const std::shared_ptr<ParentContainer>&) override;
    size_t get_min_height() const override;
    size_t get_min_width() const override;
    void handle_ready() override;
    void handle_modify(const miral::WindowSpecification&) override;
    void handle_request_move(const MirInputEvent* input_event) override;
    void handle_raise() override;
    bool resize(Direction direction, int pixels) override;
    bool set_size(const std::optional<int>& width, const std::optional<int>& height) override;
    bool toggle_fullscreen() override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout(bool cycle_thru_all) override;
    void on_open() override;
    void on_focus_gained() override;
    void on_focus_lost() override;
    void on_move_to(const geom::Point& top_left) override;
    void on_resize(const geom::Size& size) override;
    mir::geometry::Rectangle confirm_placement(MirWindowState, const mir::geometry::Rectangle&) override;
    std::shared_ptr<AbstractWorkspace> get_workspace() const override;
    void set_workspace(const std::shared_ptr<AbstractWorkspace>&) override;
    std::shared_ptr<AbstractOutput> get_output() const override;
    glm::mat4 get_transform() const override;
    void set_transform(glm::mat4 transform) override;
    void set_workspace_transform(const glm::mat4& transform) override;
    void set_workspace_alpha(float a) override;
    glm::mat4 get_workspace_transform() const override;
    glm::mat4 get_output_transform() const override;
    void set_alpha(const float alpha) override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t) override;
    bool is_focused() const override;
    bool is_fullscreen() const override;
    std::optional<miral::Window> window() const override;
    bool select_next(Direction) override;
    bool pinned() const override;
    bool pinned(bool) override;
    bool move(Direction) override;
    bool move_by(Direction, int pixels) override;
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
    bool matches(const ContainerScope&) const override;
    nlohmann::json to_json(bool is_workspace_visible) const override;
    std::optional<PluginHandle> plugin_handle() const override;

private:
    void rerender();

    PluginHandle plugin_handle_;
    miral::Window window_;
    std::optional<MirWindowState> cached;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> compositor_state;
    std::weak_ptr<AbstractWorkspace> workspace_;
    float alpha_ = 1.f;
    glm::mat4 transform_ = glm::mat4(1.f);
    glm::mat4 workspace_transform_ = glm::mat4(1.f);
    float workspace_alpha_ = 1.f;
    uint32_t handle_ = 0;
    bool is_focused_ = false;
    RenderDataManagerId render_id = -1;
};
}

#endif