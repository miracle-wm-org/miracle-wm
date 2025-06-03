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

#ifndef MOCK_COMMAND_CONTROLLER_H
#define MOCK_COMMAND_CONTROLLER_H

#include "command_controller.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{
    class MockCommandController : public AbstractCommandController
    {
    public:
        MOCK_METHOD(bool, try_request_horizontal, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_request_vertical, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_toggle_layout, (bool, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_cycle_through_request_types, (std::vector<LayoutRequestType> const&, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(void, try_toggle_resize_mode, (), (override));
        MOCK_METHOD(bool, try_resize, (Direction, int, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_resize_ppt, (Direction, float, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_set_size, (std::optional<int> const&, bool, std::optional<int> const&, bool, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_by_direction, (Direction, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_by_pixels, (Direction, int, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_by_ppt, (Direction, float, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to, (float, bool, float, bool, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_center_of_active_output, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_absolute_center, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_swap, (std::vector<ContainerScope> const&, ContainerScope), (override));
        MOCK_METHOD(bool, try_move_to_cursor, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select, (Direction, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_parent, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_child, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_prev, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_next, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_floating, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_tiling, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_select_toggle, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_close_window, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, quit, (), (override));
        MOCK_METHOD(bool, try_toggle_fullscreen, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, select_workspace, (int, bool), (override));
        MOCK_METHOD(bool, select_workspace, (std::string const&, bool), (override));
        MOCK_METHOD(bool, select_workspace_with_scope, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, next_workspace, (), (override));
        MOCK_METHOD(bool, prev_workspace, (), (override));
        MOCK_METHOD(bool, back_and_forth_workspace, (), (override));
        MOCK_METHOD(bool, next_workspace_on_output, (), (override));
        MOCK_METHOD(bool, prev_workspace_on_output, (), (override));
        MOCK_METHOD(bool, try_move_to_workspace, (std::vector<ContainerScope> const&, int, bool), (override));
        MOCK_METHOD(bool, try_move_to_workspace_named, (std::vector<ContainerScope> const&, std::string const&, bool), (override));
        MOCK_METHOD(bool, try_move_to_current_workspace, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_next_workspace, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_prev_workspace, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_back_and_forth, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_scratchpad, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, show_scratchpad, (), (override));
        MOCK_METHOD(bool, toggle_floating, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, toggle_pinned_to_workspace, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, set_is_pinned, (bool, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, toggle_tabbing, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, toggle_stacking, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, set_layout, (LayoutScheme, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, set_layout_default, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(void, move_cursor_to_output, (OutputInterface const&), (override));
        MOCK_METHOD(bool, try_select_next_output, (), (override));
        MOCK_METHOD(bool, try_select_prev_output, (), (override));
        MOCK_METHOD(bool, try_select_output, (Direction), (override));
        MOCK_METHOD(bool, try_select_output, (std::vector<std::string> const&), (override));
        MOCK_METHOD(bool, try_move_to_output_by_direction, (Direction, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_current_output, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_primary_output, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_nonprimary_output, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_next_output, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, try_move_to_output_by_name_list, (std::vector<std::string> const&, std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(bool, reload_config, (), (override));
        MOCK_METHOD(void, set_mode, (WindowManagerMode), (override));
        MOCK_METHOD(void, select_container, (std::shared_ptr<Container> const&), (override));
        MOCK_METHOD(void, mark, (std::vector<ContainerScope> const&, std::string const&, bool, bool), (override));
        MOCK_METHOD(void, unmark, (std::vector<ContainerScope> const&, std::string const&), (override));
        MOCK_METHOD(void, unmark_all, (std::vector<ContainerScope> const&), (override));
        MOCK_METHOD(std::unordered_set<std::string>, get_all_marks, (), (const, override));
        MOCK_METHOD(nlohmann::json, to_json, (), (const, override));
        MOCK_METHOD(nlohmann::json, outputs_json, (), (const, override));
        MOCK_METHOD(nlohmann::json, workspaces_json, (), (const, override));
        MOCK_METHOD(nlohmann::json, workspace_to_json, (uint32_t), (const, override));
        MOCK_METHOD(nlohmann::json, mode_to_json, (), (const, override));
    };
}
}

#endif // MOCK_COMMAND_CONTROLLER_H
