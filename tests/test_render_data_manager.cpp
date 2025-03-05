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

#include "mock_container.h"
#include "render_data_manager.h"
#include <gtest/gtest.h>

using namespace miracle;

class RenderDataManagerTest : public testing::Test
{
public:
    RenderDataManager render_data_manager;
};

TEST_F(RenderDataManagerTest, ValuesArePopulatedWhenContainerAdded)
{
    render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f) });

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
}

TEST_F(RenderDataManagerTest, CanChangeTransform)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f) });

    render_data_manager.transform_change(id, glm::mat4(2.f));

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(2.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
}

TEST_F(RenderDataManagerTest, CanChangeWorkspaceTransform)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f) });

    render_data_manager.workspace_transform_change(id, glm::mat4(2.f));

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(2.f));
}

TEST_F(RenderDataManagerTest, CanChangeFocus)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f) });

    render_data_manager.focus_change(id, false);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_FALSE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
}

class RenderDataManagerParameterizedTest : public RenderDataManagerTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(RenderDataManagerParameterizedTest, can_add_many_containers)
{
    int value = GetParam();
    for (int i = 0; i < value; i++)
    {
        render_data_manager.add({ .surface = nullptr,
            .needs_outline = true,
            .is_focused = true,
            .transform = glm::mat4(1.f),
            .workspace_transform = glm::mat4(1.f) });
    }

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), value);
}

INSTANTIATE_TEST_SUITE_P(
    RenderDataManagerParameterizedTest,
    RenderDataManagerParameterizedTest,
    ::testing::Values(1, 2, 8, 64, 128, 256, 512, 1024));
