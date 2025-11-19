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

#ifndef MIRACLE_WM_COMMAND_CONTROLLER_H
#define MIRACLE_WM_COMMAND_CONTROLLER_H

#include "compositor_state.h"
#include "container_scope.h"
#include "direction.h"
#include "output_interface.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_set>

namespace miral
{
class MirRunner;
}

namespace miracle
{
class Scratchpad;
class ModeObserverRegistrar;
class OutputManager;

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
    virtual void move_cursor_to_output(OutputInterface const&) = 0;
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
};

class CommandControllerInterface
{
public:
    virtual ~CommandControllerInterface() = default;
    virtual void quit() = 0;
};

class CommandController : public AbstractCommandController
{
public:
    CommandController(
        std::shared_ptr<Config> const& config,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<WorkspaceManager> const& workspace_manager,
        std::shared_ptr<ModeObserverRegistrar> const& mode_observer_registrar,
        std::unique_ptr<CommandControllerInterface> interface,
        std::shared_ptr<Scratchpad> const& scratchpad,
        std::shared_ptr<OutputManager> const& output_manager);

    bool try_request_horizontal(std::vector<ContainerScope> const& scope) override;
    bool try_request_vertical(std::vector<ContainerScope> const& scope) override;
    bool try_toggle_layout(bool cycle_through_all, std::vector<ContainerScope> const& scope) override;
    bool try_cycle_through_request_types(std::vector<LayoutRequestType> const& request_types, std::vector<ContainerScope> const& scope) override;
    void try_toggle_resize_mode() override;
    bool try_resize(Direction direction, int pixels, std::vector<ContainerScope> const& scope) override;
    bool try_resize_ppt(Direction direction, float ppt, std::vector<ContainerScope> const& scope) override;
    bool try_set_size(std::optional<int> const& width, bool is_width_ppt, std::optional<int> const& height, bool is_height_ppt, std::vector<ContainerScope> const& scope) override;
    bool try_move_by_direction(Direction direction, std::vector<ContainerScope> const& scope) override;
    bool try_move_by_pixels(Direction direction, int pixels, std::vector<ContainerScope> const& scope) override;
    bool try_move_by_ppt(Direction direction, float ppt, std::vector<ContainerScope> const& scope) override;
    bool try_move_to(float x, bool is_x_ppt, float y, bool is_y_ppt, std::vector<ContainerScope> const& scope) override;
    bool try_move_to_center_of_active_output(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_absolute_center(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_cursor(std::vector<ContainerScope> const& scope) override;
    bool try_swap(std::vector<ContainerScope> const& scope, ContainerScope swap_with_scope) override;
    bool try_select(std::vector<ContainerScope> const& scope) override;
    bool try_select(Direction direction, std::vector<ContainerScope> const& scope) override;
    bool try_select_parent(std::vector<ContainerScope> const& scope) override;
    bool try_select_child(std::vector<ContainerScope> const& scope) override;
    bool try_select_prev(std::vector<ContainerScope> const& scope) override;
    bool try_select_next(std::vector<ContainerScope> const& scope) override;
    bool try_select_floating(std::vector<ContainerScope> const& scope) override;
    bool try_select_tiling(std::vector<ContainerScope> const& scope) override;
    bool try_select_toggle(std::vector<ContainerScope> const& scope) override;
    bool try_close_window(std::vector<ContainerScope> const& scope) override;
    bool quit() override;
    bool try_toggle_fullscreen(std::vector<ContainerScope> const& scope) override;
    bool select_workspace(int number, bool allow_back_and_forth) override;
    bool select_workspace(std::string const& name, bool allow_back_and_forth) override;
    bool select_workspace_with_scope(std::vector<ContainerScope> const& scope) override;
    bool next_workspace() override;
    bool prev_workspace() override;
    bool back_and_forth_workspace() override;
    bool next_workspace_on_output() override;
    bool prev_workspace_on_output() override;
    bool try_move_to_workspace(std::vector<ContainerScope> const& scope, int number, bool allow_back_and_forth) override;
    bool try_move_to_workspace_named(std::vector<ContainerScope> const& scope, std::string const&, bool allow_back_and_forth) override;
    bool try_move_to_current_workspace(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_next_workspace(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_prev_workspace(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_back_and_forth(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_scratchpad(std::vector<ContainerScope> const& scope) override;
    bool show_scratchpad() override;
    bool toggle_floating(std::vector<ContainerScope> const& scope) override;
    bool toggle_pinned_to_workspace(std::vector<ContainerScope> const& scope) override;
    bool set_is_pinned(bool, std::vector<ContainerScope> const& scope) override;
    bool toggle_tabbing(std::vector<ContainerScope> const& scope) override;
    bool toggle_stacking(std::vector<ContainerScope> const& scope) override;
    bool set_layout(LayoutScheme scheme, std::vector<ContainerScope> const& scope) override;
    bool set_layout_default(std::vector<ContainerScope> const& scope) override;
    void move_cursor_to_output(OutputInterface const&) override;
    bool try_select_next_output() override;
    bool try_select_prev_output() override;
    bool try_select_output(Direction direction) override;
    bool try_select_output(std::vector<std::string> const& names) override;
    bool try_move_to_output_by_direction(Direction direction, std::vector<ContainerScope> const& scope) override;
    bool try_move_to_current_output(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_primary_output(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_nonprimary_output(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_next_output(std::vector<ContainerScope> const& scope) override;
    bool try_move_to_output_by_name_list(std::vector<std::string> const& names, std::vector<ContainerScope> const& scope) override;
    bool try_move_to_mark(std::string const& mark, std::vector<ContainerScope> const& scope) override;
    bool reload_config() override;
    void set_mode(WindowManagerMode mode) override;
    void select_container(std::shared_ptr<Container> const&) override;
    void mark(
        std::vector<ContainerScope> const& scope,
        std::string const& mark,
        bool add,
        bool toggle) override;
    void unmark(
        std::vector<ContainerScope> const& scope,
        std::string const& mark) override;
    void unmark_all(std::vector<ContainerScope> const& scope) override;
    std::unordered_set<std::string> get_all_marks() const override;
    bool rename_selected_workspace(WorkspaceIdentifier const& new_identifier) override;
    bool rename_existing_workspace(WorkspaceIdentifier const& existing_identifier, WorkspaceIdentifier const& new_identifier) override;
    bool set_inner_gaps(uint32_t px, GapsChangeType type, bool current_workspace_only) override;
    bool set_outer_gaps(uint32_t px, OuterGapsChange outer_gaps_change, GapsChangeType, bool current_workspace_only) override;
    bool try_move_workspace_to_output(OutputSelection selection) override;
    bool try_move_workspace_to_outputs_by_name(std::vector<std::string> const& outputs) override;
    [[nodiscard]] nlohmann::json to_json() const override;
    [[nodiscard]] nlohmann::json outputs_json() const override;
    [[nodiscard]] nlohmann::json workspaces_json() const override;
    [[nodiscard]] nlohmann::json workspace_to_json(uint32_t) const override;
    [[nodiscard]] nlohmann::json mode_to_json() const override;

private:
    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<ModeObserverRegistrar> mode_observer_registrar;
    std::unique_ptr<CommandControllerInterface> interface;
    std::shared_ptr<Scratchpad> scratchpad_;
    std::shared_ptr<OutputManager> output_manager;

    bool can_move_container() const;
    bool can_set_layout() const;

    /// Given the provided scope, this method will figure out what containers meet that criteria.
    std::vector<std::shared_ptr<Container>> resolve_scope(std::vector<ContainerScope> const&);

    /// Floats the container and returns the new [ParentContainer] of that container.
    std::shared_ptr<ParentContainer> toggle_floating_internal(std::shared_ptr<Container> const& container);
};
}

#endif // MIRACLE_WM_COMMAND_CONTROLLER_H
