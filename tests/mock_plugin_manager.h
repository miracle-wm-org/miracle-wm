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

#ifndef MOCK_PLUGIN_MANAGER_H
#define MOCK_PLUGIN_MANAGER_H

#include "plugin_bridge.h"
#include "plugin_manager.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{
    class MockPluginManager : public PluginManager
    {
    public:
        MOCK_METHOD(void, initialize, (std::unique_ptr<PluginBridge>), (override));
        MOCK_METHOD(PluginLoadResult, load_wasm_module, (std::string const&, std::string const&), (override));
        MOCK_METHOD(bool, unload_wasm_module, (PluginHandle), (override));
        MOCK_METHOD(void, unload_all, (), (override));
        MOCK_METHOD((std::optional<miracle_plugin_animation_frame_result_t>), animate, (AnimationData const&, float), (override));
        MOCK_METHOD(void, custom_animate, (PluginHandle, uint32_t, float, float), (override));
        MOCK_METHOD((std::optional<PluginWindowPlacement>), place_new_window, (miral::ApplicationInfo const&, miral::WindowSpecification const&, uint64_t), (override));
        MOCK_METHOD(void, window_deleted, (miral::WindowInfo const&), (override));
        MOCK_METHOD(void, window_focused, (miral::WindowInfo const&), (override));
        MOCK_METHOD(void, window_unfocused, (miral::WindowInfo const&), (override));
        MOCK_METHOD(void, workspace_created, (uint32_t), (override));
        MOCK_METHOD(void, workspace_removed, (uint32_t), (override));
        MOCK_METHOD(void, workspace_focused, ((std::optional<uint32_t>), uint32_t), (override));
        MOCK_METHOD(void, workspace_area_changed, (uint32_t), (override));
        MOCK_METHOD(void, window_workspace_changed, (miral::WindowInfo const&, uint32_t), (override));
        MOCK_METHOD(bool, handle_keyboard_event, (MirKeyboardEvent const&), (override));
        MOCK_METHOD(bool, handle_pointer_event, (MirPointerEvent const&), (override));
        MOCK_METHOD(PluginConfigData, configure, (), (override));
        MOCK_METHOD((std::optional<std::string>), handle_plugin_command, (std::string const&, std::string const&), (override));
    };
}
}

#endif // MOCK_PLUGIN_MANAGER_H
