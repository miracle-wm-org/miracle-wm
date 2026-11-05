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

TEST_F(RenderDataManagerTest, CanChangeGeometryShaderAndUniforms)
{
    auto id = render_data_manager.add(RenderData {});
    get();

    float const lag[] = { 12.f, -3.f };
    auto const uniform = ShaderUniform::from_bytes("u_lag", ShaderUniform::Type::vec2, lag);
    render_data_manager.geometry_shader_id_change(id, 7);
    ASSERT_TRUE(render_data_manager.shader_uniform_set(id, uniform));

    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].geometry_shader_id, std::optional<uint8_t> { 7 });
    ASSERT_EQ(result[0].shader_uniforms, std::vector { uniform });
    ASSERT_EQ(result[0].shader_uniforms[0].floats[0], 12.f);
    ASSERT_EQ(result[0].shader_uniforms[0].floats[1], -3.f);
    ASSERT_EQ(result[0].shader_uniforms[0].floats[2], 0.f);
}

TEST_F(RenderDataManagerTest, SettingUniformOfSameNameReplacesIt)
{
    auto id = render_data_manager.add(RenderData {});
    float const first = 1.f;
    int32_t const second = 2;
    render_data_manager.shader_uniform_set(id, ShaderUniform::from_bytes("u_value", ShaderUniform::Type::float1, &first));
    render_data_manager.shader_uniform_set(id, ShaderUniform::from_bytes("u_value", ShaderUniform::Type::int1, &second));

    auto result = get();
    ASSERT_EQ(result[0].shader_uniforms.size(), 1);
    ASSERT_EQ(result[0].shader_uniforms[0].type, ShaderUniform::Type::int1);
    ASSERT_EQ(result[0].shader_uniforms[0].integer, 2);
}

TEST_F(RenderDataManagerTest, CannotSetMoreThanMaxUniforms)
{
    auto id = render_data_manager.add(RenderData {});
    float const value = 1.f;
    for (size_t i = 0; i < ShaderUniform::max_per_window; ++i)
    {
        ASSERT_TRUE(render_data_manager.shader_uniform_set(
            id, ShaderUniform::from_bytes("u_" + std::to_string(i), ShaderUniform::Type::float1, &value)));
    }

    ASSERT_FALSE(render_data_manager.shader_uniform_set(
        id, ShaderUniform::from_bytes("u_extra", ShaderUniform::Type::float1, &value)));
    // An existing uniform may still be replaced.
    ASSERT_TRUE(render_data_manager.shader_uniform_set(
        id, ShaderUniform::from_bytes("u_0", ShaderUniform::Type::float1, &value)));
    ASSERT_EQ(get()[0].shader_uniforms.size(), ShaderUniform::max_per_window);
}

TEST_F(RenderDataManagerTest, CanClearOneUniform)
{
    auto id = render_data_manager.add(RenderData {});
    float const value = 1.f;
    render_data_manager.shader_uniform_set(id, ShaderUniform::from_bytes("u_a", ShaderUniform::Type::float1, &value));
    render_data_manager.shader_uniform_set(id, ShaderUniform::from_bytes("u_b", ShaderUniform::Type::float1, &value));
    render_data_manager.shader_uniform_clear(id, "u_a");

    auto result = get();
    ASSERT_EQ(result[0].shader_uniforms.size(), 1);
    ASSERT_EQ(result[0].shader_uniforms[0].name, "u_b");
}

TEST_F(RenderDataManagerTest, CanClearAllUniforms)
{
    auto id = render_data_manager.add(RenderData {});
    float const value = 1.f;
    render_data_manager.shader_uniform_set(id, ShaderUniform::from_bytes("u_a", ShaderUniform::Type::float1, &value));
    render_data_manager.shader_uniform_set(id, ShaderUniform::from_bytes("u_b", ShaderUniform::Type::float1, &value));
    render_data_manager.shader_uniform_clear(id, std::nullopt);

    ASSERT_TRUE(get()[0].shader_uniforms.empty());
}

TEST(ShaderUniformTest, RejectsInvalidNames)
{
    EXPECT_TRUE(ShaderUniform::is_valid_name("u_lag"));
    EXPECT_TRUE(ShaderUniform::is_valid_name("_x9"));
    EXPECT_FALSE(ShaderUniform::is_valid_name(""));
    EXPECT_FALSE(ShaderUniform::is_valid_name("9x"));
    EXPECT_FALSE(ShaderUniform::is_valid_name("u_params[0]"));
    EXPECT_FALSE(ShaderUniform::is_valid_name("u lag"));
    EXPECT_FALSE(ShaderUniform::is_valid_name("gl_Position"));
    EXPECT_FALSE(ShaderUniform::is_valid_name("alpha"));
    EXPECT_FALSE(ShaderUniform::is_valid_name("u_world_to_clip"));
    EXPECT_FALSE(ShaderUniform::is_valid_name(std::string(ShaderUniform::max_name_length + 1, 'a')));
    EXPECT_TRUE(ShaderUniform::is_valid_name(std::string(ShaderUniform::max_name_length, 'a')));
}

TEST(ShaderUniformTest, RejectsUnknownTypes)
{
    EXPECT_EQ(ShaderUniform::type_from(5), ShaderUniform::Type::mat4);
    EXPECT_EQ(ShaderUniform::type_from(6), std::nullopt);
}

TEST_F(RenderDataManagerTest, ResetShadersClearsGeometryShader)
{
    auto id = render_data_manager.add(RenderData {});
    render_data_manager.geometry_shader_id_change(id, 7);
    get();

    render_data_manager.reset_shaders({ 7 });

    copied.clear();
    auto result = get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_FALSE(result[0].geometry_shader_id.has_value());
}
