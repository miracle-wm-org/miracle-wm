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
    RenderDataManager render_data_manager;
};

TEST_F(RenderDataManagerTest, ValuesArePopulatedWhenContainerAdded)
{
    render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeTransform)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.transform_change(id, glm::mat4(2.f));

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(2.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeWorkspaceTransform)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.workspace_transform_change(id, glm::mat4(2.f));

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(2.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeFocus)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.focus_change(id, false);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_FALSE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}

TEST_F(RenderDataManagerTest, CanChangeOutputArea)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.output_area_change(id, mir::geometry::Rectangle({ 10, 10 }, { 600, 600 }));

    auto result = render_data_manager.get();
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
        render_data_manager.add({ .surface = nullptr,
            .needs_outline = true,
            .is_focused = true,
            .transform = glm::mat4(1.f),
            .workspace_transform = glm::mat4(1.f),
            .output_area = mir::geometry::Rectangle(),
            .shader_id = std::nullopt });
    }

    auto const result = render_data_manager.get();
    ASSERT_EQ(result.size(), value);
}

INSTANTIATE_TEST_SUITE_P(
    RenderDataManagerParameterizedTest,
    RenderDataManagerParameterizedTest,
    ::testing::Values(1, 2, 8, 64, 128, 256, 512, 1024));

TEST_F(RenderDataManagerTest, CanChangeGeometryShaderId)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt,
        .geometry_shader_id = std::nullopt });

    render_data_manager.geometry_shader_id_change(id, std::optional<uint8_t> { 7 });
    ASSERT_EQ(render_data_manager.get()[0].geometry_shader_id, std::optional<uint8_t> { 7 });

    render_data_manager.geometry_shader_id_change(id, std::nullopt);
    ASSERT_EQ(render_data_manager.get()[0].geometry_shader_id, std::nullopt);
}

TEST_F(RenderDataManagerTest, ResetShadersClearsBothFragmentAndGeometryIds)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::optional<uint8_t> { 5 },
        .geometry_shader_id = std::optional<uint8_t> { 6 } });

    // Removing only id 6 clears the geometry shader but leaves the fragment shader.
    render_data_manager.reset_shaders({ 6 });
    ASSERT_EQ(render_data_manager.get()[0].shader_id, std::optional<uint8_t> { 5 });
    ASSERT_EQ(render_data_manager.get()[0].geometry_shader_id, std::nullopt);

    // Removing id 5 then clears the fragment shader too.
    render_data_manager.reset_shaders({ 5 });
    ASSERT_EQ(render_data_manager.get()[0].shader_id, std::nullopt);
}

TEST_F(RenderDataManagerTest, CanChangeNeedsOutline)
{
    auto id = render_data_manager.add({ .surface = nullptr,
        .needs_outline = true,
        .is_focused = true,
        .transform = glm::mat4(1.f),
        .workspace_transform = glm::mat4(1.f),
        .output_area = mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }),
        .shader_id = std::nullopt });

    render_data_manager.needs_outline_change(id, false);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_FALSE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].output_area, mir::geometry::Rectangle({ 0, 0 }, { 400, 300 }));
}
