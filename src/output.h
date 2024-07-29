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

#ifndef MIRACLE_SCREEN_H
#define MIRACLE_SCREEN_H

#include "animator.h"
#include "direction.h"
#include "miral/window.h"
#include "tiling_window_tree.h"

#include "workspace.h"
#include <memory>
#include <miral/minimal_window_manager.h>
#include <miral/output.h>

namespace miracle
{
class WorkspaceManager;
class MiracleConfig;
class WindowManagerToolsWindowController;
class CompositorState;
class Animator;

class Output
{
public:
    Output(
        miral::Output const& output,
        WorkspaceManager& workspace_manager,
        geom::Rectangle const& area,
        miral::WindowManagerTools const& tools,
        miral::MinimalWindowManager& floating_window_manager,
        CompositorState& state,
        std::shared_ptr<MiracleConfig> const& options,
        WindowController&,
        Animator&);
    ~Output() = default;

    [[nodiscard]] int get_active_workspace_num() const { return active_workspace; }
    [[nodiscard]] std::shared_ptr<Workspace> const& get_active_workspace() const;
    bool handle_pointer_event(MirPointerEvent const* event);
    ContainerType allocate_position(miral::ApplicationInfo const& app_info, miral::WindowSpecification& requested_specification, ContainerType hint = ContainerType::none);
    [[nodiscard]] std::shared_ptr<Container> advise_new_window(miral::WindowInfo const& window_info, ContainerType type) const;
    void handle_window_ready(
        miral::WindowInfo& window_info,
        std::shared_ptr<miracle::Container> const& metadata) const;
    void advise_focus_gained(std::shared_ptr<miracle::Container> const& metadata);
    void advise_focus_lost(std::shared_ptr<miracle::Container> const& metadata);
    void advise_delete_window(std::shared_ptr<miracle::Container> const& metadata);
    void advise_move_to(std::shared_ptr<miracle::Container> const& metadata, geom::Point top_left);
    void handle_request_move(std::shared_ptr<miracle::Container> const& metadata, const MirInputEvent* input_event);
    void handle_request_resize(
        std::shared_ptr<miracle::Container> const& metadata,
        const MirInputEvent* input_event,
        MirResizeEdge edge);
    void handle_modify_window(std::shared_ptr<miracle::Container> const& metadata, const miral::WindowSpecification& modifications);
    void handle_raise_window(std::shared_ptr<miracle::Container> const& metadata);
    mir::geometry::Rectangle confirm_placement_on_display(
        std::shared_ptr<miracle::Container> const& metadata,
        MirWindowState new_state,
        const mir::geometry::Rectangle& new_placement);
    bool select_window_from_point(int x, int y) const;
    void select_window(miral::Window const&);
    void advise_new_workspace(int workspace);
    void advise_workspace_deleted(int workspace);
    bool advise_workspace_active(int workspace);
    std::vector<std::shared_ptr<Workspace>> const& get_workspaces() const { return workspaces; }
    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);
    bool point_is_in_output(int x, int y);
    void close_active_window();
    bool resize_active_window(Direction direction) const;
    bool select(Direction direction) const;
    bool move_active_window(Direction direction);
    bool move_active_window_by_amount(Direction direction, int pixels);
    bool move_active_window_to(int x, int y);
    void request_vertical_layout() const;
    void request_horizontal_layout() const;
    void toggle_layout() const;
    bool toggle_fullscreen() const;
    void toggle_pinned_to_workspace();
    void set_is_pinned(bool is_pinned);
    void update_area(geom::Rectangle const& area);
    [[nodiscard]] std::vector<miral::Window> collect_all_windows() const;
    void request_toggle_active_float();

    /// Gets the relative position of the current rectangle (e.g. the active
    /// rectangle with be at position (0, 0))
    [[nodiscard]] geom::Rectangle get_workspace_rectangle(int workspace) const;

    /// Immediately requests that the provided window be added to the output
    /// with the provided type. This is a deviation away from the typical
    /// window-adding flow where you first call 'place_new_window' followed
    /// by 'advise_new_window'.
    void add_immediately(miral::Window& window, ContainerType hint = ContainerType::none);

    geom::Rectangle const& get_area() { return area; }
    [[nodiscard]] std::vector<miral::Zone> const& get_app_zones() const { return application_zone_list; }
    miral::Output const& get_output() { return output; }
    [[nodiscard]] bool is_active() const { return is_active_; }
    void set_is_active(bool new_is_active) { is_active_ = new_is_active; }
    [[nodiscard]] CompositorState const& get_state() const { return state; }

    [[nodiscard]] glm::mat4 get_transform() const;
    void set_transform(glm::mat4 const& in);
    void set_position(glm::vec2 const&);
    [[nodiscard]] glm::vec2 const& get_position() const;

private:
    miral::Output output;
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    miral::MinimalWindowManager& floating_window_manager;
    CompositorState& state;
    geom::Rectangle area;
    std::shared_ptr<MiracleConfig> config;
    WindowController& window_controller;
    Animator& animator;
    int active_workspace = -1;
    std::vector<std::shared_ptr<Workspace>> workspaces;
    std::vector<miral::Zone> application_zone_list;
    bool is_active_ = false;
    AnimationHandle handle;
    bool has_clicked_floating_window = false;

    /// The position of the output for scrolling across workspaces
    glm::vec2 position_offset = glm::vec2(0.f);

    /// The transform applied to the workspace
    glm::mat4 transform = glm::mat4(1.f);

    /// A matrix resulting from combining position + transform
    glm::mat4 final_transform = glm::mat4(1.f);
};

}

#endif
