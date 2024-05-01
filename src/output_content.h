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

#include "miral/window.h"
#include "tiling_window_tree.h"
#include "window_metadata.h"
#include "workspace_content.h"
#include <memory>
#include <miral/minimal_window_manager.h>
#include <miral/output.h>

namespace miracle
{
class WorkspaceManager;
class MiracleConfig;
class WindowManagerToolsTilingInterface;

class OutputContent
{
public:
    OutputContent(
        miral::Output const& output,
        WorkspaceManager& workspace_manager,
        geom::Rectangle const& area,
        miral::WindowManagerTools const& tools,
        miral::MinimalWindowManager& floating_window_manager,
        std::shared_ptr<MiracleConfig> const& options,
        TilingInterface&);
    ~OutputContent() = default;

    [[nodiscard]] std::shared_ptr<TilingWindowTree> get_active_tree() const;
    [[nodiscard]] int get_active_workspace_num() const { return active_workspace; }
    [[nodiscard]] std::shared_ptr<WorkspaceContent> get_active_workspace() const;
    bool handle_pointer_event(MirPointerEvent const* event);
    WindowType allocate_position(miral::WindowSpecification& requested_specification);
    std::shared_ptr<WindowMetadata> advise_new_window(miral::WindowInfo const& window_info, WindowType type);
    void handle_window_ready(miral::WindowInfo& window_info, std::shared_ptr<miracle::WindowMetadata> const& metadata);
    void advise_focus_gained(std::shared_ptr<miracle::WindowMetadata> const& metadata);
    void advise_focus_lost(std::shared_ptr<miracle::WindowMetadata> const& metadata);
    void advise_delete_window(std::shared_ptr<miracle::WindowMetadata> const& metadata);
    void advise_move_to(std::shared_ptr<miracle::WindowMetadata> const& metadata, geom::Point top_left);
    void handle_request_move(std::shared_ptr<miracle::WindowMetadata> const& metadata, const MirInputEvent* input_event);
    void handle_request_resize(
        std::shared_ptr<miracle::WindowMetadata> const& metadata,
        const MirInputEvent* input_event,
        MirResizeEdge edge);
    void advise_state_change(std::shared_ptr<miracle::WindowMetadata> const& metadata, MirWindowState state);
    void handle_modify_window(std::shared_ptr<miracle::WindowMetadata> const& metadata, const miral::WindowSpecification& modifications);
    void handle_raise_window(std::shared_ptr<miracle::WindowMetadata> const& metadata);
    mir::geometry::Rectangle confirm_placement_on_display(
        std::shared_ptr<miracle::WindowMetadata> const& metadata,
        MirWindowState new_state,
        const mir::geometry::Rectangle& new_placement);
    void select_window_from_point(int x, int y);
    void advise_new_workspace(int workspace);
    void advise_workspace_deleted(int workspace);
    bool advise_workspace_active(int workspace);
    std::vector<std::shared_ptr<WorkspaceContent>> const& get_workspaces() const { return workspaces; }
    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);
    bool point_is_in_output(int x, int y);
    void close_active_window();
    bool resize_active_window(Direction direction);
    bool select(Direction direction);
    bool move_active_window(Direction direction);
    void request_vertical();
    void request_horizontal();
    void toggle_resize_mode();
    void toggle_fullscreen();
    void toggle_pinned_to_workspace();
    void update_area(geom::Rectangle const& area);
    std::vector<miral::Window> collect_all_windows() const;
    void request_toggle_active_float();

    /// Immediately requests that the provided window be added to the output
    /// with the provided type. This is a deviation away from the typical
    /// window-adding flow where you first call 'allocate_position' followed
    /// by 'advise_new_window'.
    void add_immediately(miral::Window& window);

    geom::Rectangle const& get_area() { return area; }
    std::vector<miral::Zone> const& get_app_zones() { return application_zone_list; }
    miral::Output const& get_output() { return output; }
    [[nodiscard]] bool is_active() const { return is_active_; }
    void set_is_active(bool new_is_active) { is_active_ = new_is_active; }
    miral::Window get_active_window() { return active_window; }

private:
    miral::Output output;
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    miral::MinimalWindowManager& floating_window_manager;
    geom::Rectangle area;
    std::shared_ptr<MiracleConfig> config;
    TilingInterface& node_interface;
    int active_workspace = -1;
    std::vector<std::shared_ptr<WorkspaceContent>> workspaces;
    std::vector<miral::Zone> application_zone_list;
    bool is_active_ = false;
    miral::Window active_window;
};

}

#endif
