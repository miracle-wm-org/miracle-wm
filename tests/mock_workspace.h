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

#ifndef MIRACLE_WM_MOCK_WORKSPACE_H
#define MIRACLE_WM_MOCK_WORKSPACE_H

#include "abstract_workspace.h"
#include <functional>
#include <gmock/gmock.h>
#include <memory>
#include <optional>
#include <string>

namespace miracle
{
namespace test
{
    class MockWorkspace : public AbstractWorkspace
    {
    public:
        MOCK_METHOD(void, recalculate_area, (), (override));

        MOCK_METHOD(void, delete_container, (std::shared_ptr<Container> const& container), (override));
        MOCK_METHOD(bool, move_container, (Direction direction, Container&), (override));
        MOCK_METHOD(void, show, (geom::Point const&), (override));
        MOCK_METHOD(void, hide, (geom::Point const&), (override));
        MOCK_METHOD(bool, begin_preview, (), (override));
        MOCK_METHOD(void, end_preview, (), (override));
        MOCK_METHOD(bool, add_to_root, (Container&), (override));

        MOCK_METHOD(bool, for_each_window,
            (std::function<bool(std::shared_ptr<WindowContainer>)> const&), (const, override));

        MOCK_METHOD(void, advise_focus_gained, (std::shared_ptr<Container> const& container), (override));

        MOCK_METHOD(void, select_window, (), (override));

        MOCK_METHOD(std::shared_ptr<AbstractOutput>, get_output, (), (const, override));

        MOCK_METHOD(void, set_output, (std::shared_ptr<AbstractOutput> const&), (override));

        MOCK_METHOD(bool, is_empty, (), (const, override));
        MOCK_METHOD(void, graft, (std::shared_ptr<Container> const&), (override));

        MOCK_METHOD(uint32_t, id, (), (const, override));
        MOCK_METHOD(std::optional<int>, num, (), (const, override));
        MOCK_METHOD(void, num, (std::optional<int>), (override));
        MOCK_METHOD(nlohmann::json, get_workspaces_json, (bool), (const, override));
        MOCK_METHOD(nlohmann::json, to_json, (bool), (const, override));
        MOCK_METHOD(std::optional<std::string> const&, name, (), (const, override));
        MOCK_METHOD(void, name, (std::optional<std::string> const&), (override));
        MOCK_METHOD(std::string, display_name, (), (const, override));
        MOCK_METHOD(std::shared_ptr<ParentContainer>, get_root, (), (const, override));

        MOCK_METHOD(mir::geometry::Rectangle, area, (), (const, override));

        MOCK_METHOD(std::optional<Gaps>, outer_gaps, (), (const, override));
        MOCK_METHOD(void, outer_gaps, (std::optional<Gaps> const&), (override));

        MOCK_METHOD(std::optional<Gaps>, inner_gaps, (), (const, override));
        MOCK_METHOD(void, inner_gaps, (std::optional<Gaps> const&), (override));

        MOCK_METHOD(void, transform, (glm::mat4 const&), (override));
        MOCK_METHOD(glm::mat4, transform, (), (const, override));

        MOCK_METHOD(void, alpha, (float), (override));
        MOCK_METHOD(float, alpha, (), (const, override));

        MOCK_METHOD(ParentContainer*, get_layout_container, (), (const, override));

        MOCK_METHOD(void, add_other_container, (std::shared_ptr<Container> const&, bool), (override));
        MOCK_METHOD(void, remove_other_container, (std::shared_ptr<Container> const&), (override));
    };
}
}

#endif // MIRACLE_WM_MOCK_WORKSPACE_H
