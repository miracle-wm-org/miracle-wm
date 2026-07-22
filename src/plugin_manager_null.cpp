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

#include "plugin_manager.h"

#include <memory>

namespace miracle
{
namespace
{
    /// A PluginManager that does nothing. Used when the plugin feature is compiled
    /// out, or as a safe fallback when the plugin module fails to load, so the rest
    /// of the compositor keeps working (and rendering) without plugins.
    class NullPluginManager final : public PluginManager
    {
    public:
        void initialize(std::unique_ptr<PluginBridge>) override { }
        PluginLoadResult load_wasm_module(std::string const&, std::string const&) override
        {
            return PluginLoadResult { .success = false, .error = "Plugin system is not available" };
        }
        bool unload_wasm_module(PluginHandle) override { return false; }
        void unload_all() override { }
        std::optional<miracle_plugin_animation_frame_result_t> animate(
            AnimationData const&, float) override
        {
            return std::nullopt;
        }
        void custom_animate(PluginHandle, uint32_t, float, float) override { }
        std::optional<PluginWindowPlacement> place_new_window(
            miral::ApplicationInfo const&,
            miral::WindowSpecification const&,
            uint64_t) override
        {
            return std::nullopt;
        }
        void window_deleted(miral::WindowInfo const&) override { }
        void window_focused(miral::WindowInfo const&) override { }
        void window_unfocused(miral::WindowInfo const&) override { }
        void workspace_created(uint32_t) override { }
        void workspace_removed(uint32_t) override { }
        void workspace_focused(std::optional<uint32_t>, uint32_t) override { }
        void workspace_area_changed(uint32_t) override { }
        void window_workspace_changed(miral::WindowInfo const&, uint32_t) override { }
        bool handle_keyboard_event(MirKeyboardEvent const&) override { return false; }
        bool handle_pointer_event(MirPointerEvent const&) override { return false; }
        PluginConfigData configure() override { return PluginConfigData {}; }
        std::optional<std::string> handle_plugin_command(
            std::string const&, std::string const&) override
        {
            return std::nullopt;
        }
    };
}

std::shared_ptr<PluginManager> make_null_plugin_manager()
{
    return std::make_shared<NullPluginManager>();
}
}
