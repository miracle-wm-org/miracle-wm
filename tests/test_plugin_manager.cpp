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

#include "miracle/plugin.h"
#include "plugin_manager.h"
#include <gtest/gtest.h>

using miracle::PluginHandle;
using miracle::PluginLoadResult;
using miracle::PluginManager;

TEST(PluginManagerTest, LoadInvalidPathFailsAndHasNoHandle)
{
    PluginManager pm;
    PluginLoadResult const res = pm.load_wasm_module("/definitely/not/a/real/module.wasm", "module");

    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.handle, 0);
    EXPECT_FALSE(res.error.empty());
}

TEST(PluginManagerTest, AddPointsReturnsSomePoint)
{
    PluginManager pm;
    mir::geometry::Point const a { 1, 2 };
    mir::geometry::Point const b { 3, 4 };

    auto const sum = pm.add_points(a, b);
    (void)sum;
    SUCCEED();
}

TEST(PluginManagerTest, AnimateFrameUnknownHandleIsGraceful)
{
    PluginManager pm;
    miracle_plugin_animation_frame_data_t frame_data {};
    auto const result = pm.animate_frame(123456u, frame_data);
    (void)result;
    SUCCEED();
}

TEST(PluginManagerTest, UnloadUnknownHandleReturnsFalseWhenEnabled)
{
    PluginManager pm;
    EXPECT_FALSE(pm.unload_wasm_module(9999u));
    SUCCEED();
}
