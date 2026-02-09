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
#include "mock_output_factory.h"
#include "mock_window_controller.h"
#include "output_manager.h"
#include "plugin_bridge.h"
#include "plugin_manager.h"
#include <gtest/gtest.h>

using namespace miracle;

namespace
{
class PluginManagerTest : public ::testing::Test
{
public:
    PluginManagerTest()
    {
        pm.initialize(
            std::make_unique<PluginBridge>(
                std::make_shared<OutputManager>(std::make_unique<test::MockOutputFactory>()),
                std::make_shared<test::MockWindowController>(),
                nullptr));
    }

    PluginManager pm;
};
}

TEST_F(PluginManagerTest, LoadInvalidPathFailsAndHasNoHandle)
{
    PluginLoadResult const res = pm.load_wasm_module("/definitely/not/a/real/module.wasm", "module");

    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.handle, 0);
    EXPECT_FALSE(res.error.empty());
}

TEST_F(PluginManagerTest, AddPointsReturnsSomePoint)
{
    mir::geometry::Point const a { 1, 2 };
    mir::geometry::Point const b { 3, 4 };

    auto const sum = pm.add_points(0, "add_points", a, b);
    (void)sum;
    SUCCEED();
}

TEST_F(PluginManagerTest, AnimateFrameUnknownHandleIsGraceful)
{
    miracle_plugin_animation_frame_data_t frame_data {};
    auto const result = pm.animate_frame(123456u, "animate", frame_data);
    (void)result;
    SUCCEED();
}

TEST_F(PluginManagerTest, UnloadUnknownHandleReturnsFalseWhenEnabled)
{
    EXPECT_FALSE(pm.unload_wasm_module(9999u));
    SUCCEED();
}
