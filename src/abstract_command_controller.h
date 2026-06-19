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

#ifndef MIRACLE_WM_ABSTRACT_COMMAND_CONTROLLER_H
#define MIRACLE_WM_ABSTRACT_COMMAND_CONTROLLER_H

#include "abstract_output.h"
#include "compositor_state.h"
#include "container_scope.h"
#include "direction.h"
#include "layout_scheme.h"
#include "window_allocation.h"
#include "workspace.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace miracle
{
class Container;

enum class LayoutRequestType
{
    split,
    tabbed,
    stacking,
    splitv,
    splith
};

enum class GapsChangeType
{
    set,
    plus,
    minus,
};

enum class OuterGapsChange
{
    outer,
    horizontal,
    vertical,
    top,
    right,
    bottom,
    left
};

enum class OutputSelection
{
    left,
    right,
    down,
    up,
    current,
    primary,
    nonprimary,
    next
};

/// Responsible for fielding requests from the system and forwarding
/// them to an appropriate handler. Requests can come from any thread
/// (e.g. the keyboard input thread, the ipc thread, etc.).
/// This class behaves similar to a controller pattern in a server
/// whereby requests are made on the controller that are then sent
/// to the proper subsystem for processing. In this case, the subsystem
/// may be anything from the tiles in the grid, the scratchpad, etc.
class AbstractCommandController
{
public:
    virtual ~AbstractCommandController() = default;
    virtual std::shared_ptr<WindowContainer> create_container(miral::WindowInfo const& window_info, AllocationHint const& hint) = 0;
    virtual bool try_request_horizontal(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_request_vertical(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_toggle_layout(bool cycle_through_all, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_cycle_through_request_types(std::vector<LayoutRequestType> const& request_types, std::vector<ContainerScope> const& scope) = 0;
    virtual void try_toggle_resize_mode() = 0;
    virtual bool try_resize(Direction direction, int pixels, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_resize_ppt(Direction direction, float ppt, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_set_size(std::optional<int> const& width, bool is_width_ppt, std::optional<int> const& height, bool is_height_ppt, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_by_direction(Direction direction, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_by_pixels(Direction direction, int pixels, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_by_ppt(Direction direction, float ppt, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to(float x, bool is_x_ppt, float y, bool is_y_ppt, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_center_of_active_output(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_absolute_center(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_cursor(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_swap(std::vector<ContainerScope> const& scope, ContainerScope swap_with_scope) = 0;
    virtual bool try_select(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select(Direction direction, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_parent(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_child(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_prev(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_next(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_floating(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_tiling(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_select_toggle(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_close_window(std::vector<ContainerScope> const& scope) = 0;
    virtual bool quit() = 0;
    virtual bool try_toggle_fullscreen(std::vector<ContainerScope> const& scope) = 0;
    virtual bool select_workspace(int number, bool allow_back_and_forth) = 0;
    virtual bool select_workspace(std::string const& name, bool allow_back_and_forth) = 0;
    virtual bool select_workspace_with_scope(std::vector<ContainerScope> const& scope) = 0;
    virtual bool next_workspace() = 0;
    virtual bool prev_workspace() = 0;
    virtual bool back_and_forth_workspace() = 0;
    virtual bool next_workspace_on_output() = 0;
    virtual bool prev_workspace_on_output() = 0;
    virtual bool try_move_to_workspace(std::vector<ContainerScope> const& scope, int number, bool allow_back_and_forth) = 0;
    virtual bool try_move_to_workspace_named(std::vector<ContainerScope> const& scope, std::string const&, bool allow_back_and_forth) = 0;
    virtual bool try_move_to_current_workspace(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_next_workspace(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_prev_workspace(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_back_and_forth(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_scratchpad(std::vector<ContainerScope> const& scope) = 0;
    virtual bool show_scratchpad() = 0;
    virtual bool toggle_floating(std::vector<ContainerScope> const& scope) = 0;
    virtual bool toggle_pinned_to_workspace(std::vector<ContainerScope> const& scope) = 0;
    virtual bool set_is_pinned(bool, std::vector<ContainerScope> const& scope) = 0;
    virtual bool toggle_tabbing(std::vector<ContainerScope> const& scope) = 0;
    virtual bool toggle_stacking(std::vector<ContainerScope> const& scope) = 0;
    virtual bool set_layout(LayoutScheme scheme, std::vector<ContainerScope> const& scope) = 0;
    virtual bool set_layout_default(std::vector<ContainerScope> const& scope) = 0;
    virtual void move_cursor_to_output(AbstractOutput const&) = 0;
    virtual bool try_select_next_output() = 0;
    virtual bool try_select_prev_output() = 0;
    virtual bool try_select_output(Direction direction) = 0;
    virtual bool try_select_output(std::vector<std::string> const& names) = 0;
    virtual bool try_move_to_output_by_direction(Direction direction, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_current_output(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_primary_output(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_nonprimary_output(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_next_output(std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_output_by_name_list(std::vector<std::string> const& names, std::vector<ContainerScope> const& scope) = 0;
    virtual bool try_move_to_mark(std::string const& mark, std::vector<ContainerScope> const& scope) = 0;
    virtual bool reload_config() = 0;
    virtual void set_mode(WindowManagerMode mode) = 0;
    virtual void select_container(std::shared_ptr<Container> const&) = 0;
    virtual void mark(
        std::vector<ContainerScope> const& scope,
        std::string const& mark,
        bool add,
        bool toggle)
        = 0;
    virtual void unmark(
        std::vector<ContainerScope> const& scope,
        std::string const& mark)
        = 0;
    virtual void unmark_all(std::vector<ContainerScope> const& scope) = 0;
    virtual std::unordered_set<std::string> get_all_marks() const = 0;
    virtual bool rename_selected_workspace(WorkspaceIdentifier const& new_identifier) = 0;
    virtual bool rename_existing_workspace(WorkspaceIdentifier const& existing_identifier, WorkspaceIdentifier const& new_identifier) = 0;
    virtual bool set_inner_gaps(uint32_t px, GapsChangeType type, bool current_workspace_only) = 0;
    virtual bool set_outer_gaps(uint32_t px, OuterGapsChange outer_gaps_change, GapsChangeType, bool current_workspace_only) = 0;
    virtual bool try_move_workspace_to_output(OutputSelection selection) = 0;
    virtual bool try_move_workspace_to_outputs_by_name(std::vector<std::string> const& outputs) = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    [[nodiscard]] virtual nlohmann::json outputs_json() const = 0;
    [[nodiscard]] virtual nlohmann::json workspaces_json() const = 0;
    [[nodiscard]] virtual nlohmann::json workspace_to_json(uint32_t) const = 0;
    [[nodiscard]] virtual nlohmann::json mode_to_json() const = 0;

    /// Builds the JSON consumed by the debug overlay client: the cursor
    /// position, the id of the window under the cursor, and a flat list of every
    /// window (with geometry, clip area, visibility, ...) across all outputs.
    [[nodiscard]] virtual nlohmann::json debug_state_to_json() const = 0;
};

class CommandControllerInterface
{
public:
    virtual ~CommandControllerInterface() = default;
    virtual void quit() = 0;
};

}

#endif // MIRACLE_WM_ABSTRACT_COMMAND_CONTROLLER_H
