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

#include "sampler_registry.h"
#include <gtest/gtest.h>

using namespace miracle;

class SamplerRegistryTest : public testing::Test
{
public:
    SamplerRegistry registry;
};

TEST_F(SamplerRegistryTest, RegisterSinglePassWindowShaderYieldsOnePassEntry)
{
    auto id = registry.register_window_shader({ "vec4 sample_to_rgba(vec2 tc) { return vec4(1); }" });
    ASSERT_EQ(registry.entries.size(), 1u);
    ASSERT_EQ(registry.entries[0].id, id);
    ASSERT_EQ(registry.entries[0].passes.size(), 1u);
}

TEST_F(SamplerRegistryTest, RegisterWindowShaderStoresAllPasses)
{
    std::vector<std::string> passes = {
        "vec4 sample_to_rgba(vec2 tc) { return texture2D(tex, tc); }",
        "vec4 sample_to_rgba(vec2 tc) { return texture2D(tex, tc) * 0.5; }",
        "vec4 sample_to_rgba(vec2 tc) { return texture2D(tex, tc); }",
    };
    auto id = registry.register_window_shader(passes);
    ASSERT_EQ(registry.entries.size(), 1u);
    ASSERT_EQ(registry.entries[0].id, id);
    ASSERT_EQ(registry.entries[0].passes.size(), 3u);
    ASSERT_EQ(registry.entries[0].passes[0], passes[0]);
    ASSERT_EQ(registry.entries[0].passes[1], passes[1]);
    ASSERT_EQ(registry.entries[0].passes[2], passes[2]);
}

TEST_F(SamplerRegistryTest, IdsAreMonotonicFromFive)
{
    auto id0 = registry.register_window_shader({ "vec4 sample_to_rgba(vec2 tc) { return vec4(1); }" });
    auto id1 = registry.register_window_shader({ "vec4 sample_to_rgba(vec2 tc) { return vec4(0); }" });
    auto id2 = registry.register_window_shader({ "vec4 sample_to_rgba(vec2 tc) { return vec4(0.5); }" });
    ASSERT_EQ(id0, 5);
    ASSERT_EQ(id1, 6);
    ASSERT_EQ(id2, 7);
}

TEST_F(SamplerRegistryTest, MultipleRegistrationsAccumulate)
{
    for (int i = 0; i < 8; ++i)
        registry.register_window_shader({ "vec4 sample_to_rgba(vec2 tc) { return vec4(1); }" });
    ASSERT_EQ(registry.entries.size(), 8u);
}
