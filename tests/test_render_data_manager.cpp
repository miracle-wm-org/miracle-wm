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

#include "mock_container.h"
#include "render_data_manager.h"
#include <gtest/gtest.h>

using namespace miracle;

class RenderDataManagerTest : public testing::Test
{
public:
    std::vector<RenderData> const& get()
    {
        render_data_manager.copy_if_changed(seen_generation, copied);
        return copied;
    }

    RenderDataManager render_data_manager;
    uint64_t seen_generation = 0;
    std::vector<RenderData> copied;
};

TEST_F(RenderDataManagerTest, ValuesArePopulatedWhenContainerAdded)
{
    render_data_manager.add({ .window = {},
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeTransform)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.transform_change(id, glm::mat4(2.f));

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(2.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeWorkspaceTransform)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.workspace_transform_change(id, glm::mat4(2.f));

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(2.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeFocus)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.focus_change(id, false);

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_FALSE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeOutputArea)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.output_area_change(id, mir::geometry::Rectangle({ 10, 10 }, { 600, 600 }));

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 10, 10 }, { 600, 600 }));
}

class RenderDataManagerParameterizedTest : public RenderDataManagerTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(RenderDataManagerParameterizedTest, can_add_many_containers)
{
    auto const value = GetParam();
    for (int i = 0; i < value; i++)
    {
        render_data_manager.add({ .window = {},
            .needs_outline = true,
            .is_focused = true,
            .transform = glm::mat4(1.f),
            .workspace_transform = glm::mat4(1.f),
            .output_area = mir::geometry::Rectangle(),
            .shader_id = std::nullopt });
    }

    auto const result = get();
    ASSERT_EQ(result.size(), value);
}

INSTANTIATE_TEST_SUITE_P(
    RenderDataManagerParameterizedTest,
    RenderDataManagerParameterizedTest,
    ::testing::Values(1, 2, 8, 64, 128, 256, 512, 1024));

TEST_F(RenderDataManagerTest, CanChangeNeedsOutline)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.needs_outline_change(id, false);

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_FALSE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CopyIsSkippedWhenNothingChanged)
{
    render_data_manager.add({ .window = {},
        .needs_outline = false,
        .is_focused = false,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = std::nullopt,
        .shader_id = std::nullopt });
    render_data_manager.copy_if_changed(seen_generation, copied);
    ASSERT_EQ(copied.size(), 1);

    // If copy_if_changed copies despite no changes, the cleared
    // vector is repopulated and this test fails.
    copied.clear();
    render_data_manager.copy_if_changed(seen_generation, copied);
    ASSERT_TRUE(copied.empty());
}

TEST_F(RenderDataManagerTest, CopyHappensAfterMutation)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = false,
        .is_focused = false,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = std::nullopt,
        .shader_id = std::nullopt });
    render_data_manager.copy_if_changed(seen_generation, copied);

    render_data_manager.transform_change(id, glm::mat4(2.f));
    render_data_manager.copy_if_changed(seen_generation, copied);
    ASSERT_EQ(copied.size(), 1);
    ASSERT_EQ(copied[0].transform, glm::mat4(2.f));
}

TEST_F(RenderDataManagerTest, MutationOfUnknownIdDoesNotTriggerCopy)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = false,
        .is_focused = false,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = std::nullopt,
        .shader_id = std::nullopt });
    render_data_manager.copy_if_changed(seen_generation, copied);

    render_data_manager.transform_change(id + 1, glm::mat4(2.f));
    copied.clear();
    render_data_manager.copy_if_changed(seen_generation, copied);
    ASSERT_TRUE(copied.empty());
}

TEST_F(RenderDataManagerTest, ConcurrentCallersEachObserveChanges)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = false,
        .is_focused = false,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = std::nullopt,
        .shader_id = std::nullopt });

    uint64_t other_generation = 0;
    std::vector<RenderData> other_copy;

    render_data_manager.copy_if_changed(seen_generation, copied);
    render_data_manager.copy_if_changed(other_generation, other_copy);
    ASSERT_EQ(copied.size(), 1);
    ASSERT_EQ(other_copy.size(), 1);

    render_data_manager.focus_change(id, false);
    render_data_manager.copy_if_changed(seen_generation, copied);
    render_data_manager.copy_if_changed(other_generation, other_copy);
    ASSERT_FALSE(copied[0].is_focused);
    ASSERT_FALSE(other_copy[0].is_focused);
}

TEST_F(RenderDataManagerTest, RemoveTriggersCopy)
{
    auto id = render_data_manager.add({ .window = {},
        .needs_outline = false,
        .is_focused = false,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = std::nullopt,
        .shader_id = std::nullopt });
    render_data_manager.copy_if_changed(seen_generation, copied);
    ASSERT_EQ(copied.size(), 1);

    render_data_manager.remove(id);
    render_data_manager.copy_if_changed(seen_generation, copied);
    ASSERT_TRUE(copied.empty());
}
