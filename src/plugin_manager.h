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

#ifndef MIRACLEWM_PLUGIN_MANAGER_H
#define MIRACLEWM_PLUGIN_MANAGER_H
#include "../miracle-plugin-rs/plugin.h"
#include "layout_scheme.h"
#include "plugin_handle.h"
#include <glm/glm.hpp>
#include <mir/geometry/rectangle.h>
#include <miracle/cpp/config-cpp.h>
namespace miracle
{
class Container;
class AbstractWorkspace;

struct PluginWindowPlacement
{
    struct TiledPlacement
    {
        Container* container;
        uint32_t index;
        LayoutScheme scheme;
    };

    struct FreestylePlacement
    {
        /// The rectangle of the window.
        mir::geometry::Rectangle rectangle;

        /// The layer of the window.
        MirDepthLayer layer;

        /// The workspace of the window.
        AbstractWorkspace* workspace;

        /// The plugin handle that is managing the window.
        PluginHandle handle;

        /// The initial 4x4 transform applied to the window.
        glm::mat4 transform = glm::mat4(1.f);

        /// The initial alpha (opacity) of the window.
        float alpha = 1.f;

        /// Whether the window can be resized.
        bool resizable = true;

        /// Whether the window can be moved.
        bool movable = true;
    };

    miracle_window_management_strategy_t strategy = miracle_window_management_strategy_system;
    TiledPlacement tiled;
    FreestylePlacement freestyle;
};
}

namespace miracle
{
struct AnimationData;
}

#if FEATURE_PLUGIN_SYSTEM
#include <memory>
#include <mir/geometry/point.h>
#include <mutex>
#include <vector>
#include <wasmedge/wasmedge.h>

namespace miracle
{
class PluginBridge;

struct PluginLoadResult
{
    bool success = false;
    PluginHandle handle = 0;
    std::string error;
};

class PluginManager
{
public:
    ~PluginManager();

    /// Initialize the plugin bridge.
    ///
    /// This must be called after construction.
    ///
    /// TODO: This is a slightly unfortunate thing that I may want to alleviate someday, but
    ///  for now its not a huge deal.
    void initialize(std::unique_ptr<PluginBridge> bridge);

    /// Load a WebAssembly module from the provided \p path.
    ///
    /// Callers may use the #PluginLoadResult::handle to unload the module later.
    ///
    /// \param path The filesystem path to the WebAssembly module.
    /// \param userdata_json Optional JSON-encoded user data to associate with the plugin.
    /// \returns a load result.
    PluginLoadResult load_wasm_module(std::string const& path, std::string const& userdata_json = "");

    /// Attempt to unload the WebAssembly module associated with \p handle.
    ///
    /// \returns `true` if the module was unloaded, `false` otherwise.
    bool unload_wasm_module(PluginHandle handle);

    /// Unload all loaded WebAssembly modules.
    void unload_all();

    /// Animate a frame via the plugin system.
    ///
    /// \param data The animation data to animate.
    /// \param runtime_seconds The current runtime of the animation in seconds.
    /// \returns The result of the animation frame, or none if none is set.
    std::optional<miracle_plugin_animation_frame_result_t> animate(
        AnimationData const& data, float runtime_seconds);

    /// Tick a custom animation for the given plugin.
    ///
    /// \param plugin_handle The handle of the plugin that owns this animation.
    /// \param animation_id The host-generated animation ID.
    /// \param dt The time delta in seconds since the last tick.
    /// \param elapsed_seconds The cumulative elapsed time in seconds since the animation started.
    void custom_animate(PluginHandle plugin_handle, uint32_t animation_id, float dt, float elapsed_seconds);

    /// Try to place a new window using a plugin.
    ///
    /// \param app_info The application info.
    /// \param spec The window specification.
    /// \returns the placement
    std::optional<PluginWindowPlacement> place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& spec,
        uint64_t window_id);

    /// Notify all plugins that a window has been deleted.
    ///
    /// \param window_info The window info for the deleted window.
    void window_deleted(miral::WindowInfo const& window_info);

    /// Notify all plugins that a window has been focused.
    ///
    /// \param window_info The window info for the focused window.
    void window_focused(miral::WindowInfo const& window_info);

    /// Notify all plugins that a window has been unfocused.
    ///
    /// \param window_info The window info for the unfocused window.
    void window_unfocused(miral::WindowInfo const& window_info);

    /// Notify all plugins that a workspace has been created.
    ///
    /// \param id The ID of the created workspace.
    void workspace_created(uint32_t id);

    /// Notify all plugins that a workspace has been removed.
    ///
    /// \param id The ID of the removed workspace.
    void workspace_removed(uint32_t id);

    /// Notify all plugins that a workspace has gained focus.
    ///
    /// \param previous_id The ID of the previously focused workspace, if any.
    /// \param current_id The ID of the newly focused workspace.
    void workspace_focused(std::optional<uint32_t> previous_id, uint32_t current_id);

    /// Notify all plugins that a workspace's area has changed.
    ///
    /// \param id The ID of the workspace whose area changed.
    void workspace_area_changed(uint32_t id);

    /// Notify all plugins that a window's workspace has changed.
    ///
    /// \param window_info The window info for the window that moved.
    /// \param workspace_id The ID of the new workspace.
    void window_workspace_changed(miral::WindowInfo const& window_info, uint32_t workspace_id);

    /// Check if the plugin handles a keyboard event.
    ///
    /// \param event the incoming keyboard event
    /// \returns `true` if the keyboard event was consumed, otherwise `false`
    bool handle_keyboard_event(MirKeyboardEvent const& event);

    /// Check if the plugin handles a pointer event.
    ///
    /// \param event the incoming pointer event
    /// \returns `true` if the pointer event was consumed, otherwise `false`
    bool handle_pointer_event(MirPointerEvent const& event);

    /// Call the configure() export on every loaded plugin and merge all of their
    /// results into a single PluginConfigData. Plugins that do not export configure()
    /// are silently skipped. The plugins and includes fields are never set.
    PluginConfigData configure();

    /// Data passed to host functions that need both the bridge and the manager.
    struct HostFunctionData
    {
        PluginBridge* bridge = nullptr;
        PluginManager* manager = nullptr;
    };

private:
    struct Self
    {
        template <auto DeleteFn>
        struct WasmEdgeDeleter
        {
            template <typename T>
            void operator()(T* ptr) const noexcept
            {
                if (ptr)
                {
                    DeleteFn(ptr);
                }
            }
        };

        using ConfigurePtr = std::unique_ptr<WasmEdge_ConfigureContext,
            WasmEdgeDeleter<WasmEdge_ConfigureDelete>>;

        using StorePtr = std::unique_ptr<WasmEdge_StoreContext,
            WasmEdgeDeleter<WasmEdge_StoreDelete>>;

        using VMPtr = std::unique_ptr<WasmEdge_VMContext,
            WasmEdgeDeleter<WasmEdge_VMDelete>>;

        using LoaderPtr = std::unique_ptr<WasmEdge_LoaderContext,
            WasmEdgeDeleter<WasmEdge_LoaderDelete>>;

        using ValidtorPtr = std::unique_ptr<WasmEdge_ValidatorContext,
            WasmEdgeDeleter<WasmEdge_ValidatorDelete>>;

        using ExecutorPtr = std::unique_ptr<WasmEdge_ExecutorContext,
            WasmEdgeDeleter<WasmEdge_ExecutorDelete>>;

        using ModuleInstancePtr = std::unique_ptr<WasmEdge_ModuleInstanceContext,
            WasmEdgeDeleter<WasmEdge_ModuleInstanceDelete>>;

        using FunctionTypePtr = std::unique_ptr<WasmEdge_FunctionTypeContext,
            WasmEdgeDeleter<WasmEdge_FunctionTypeDelete>>;

        struct ModuleInstance
        {
            std::shared_ptr<WasmEdge_ModuleInstanceContext> module_context;
            PluginHandle handle;
            std::string name;
        };

        explicit Self(std::unique_ptr<PluginBridge> bridge);
        ~Self();
        void create_host_module();

        std::unique_ptr<PluginBridge> bridge;
        ConfigurePtr configure_context;
        StorePtr store_context;
        LoaderPtr loader_context;
        ValidtorPtr validator_context;
        ExecutorPtr executor_context;
        ModuleInstancePtr wasi_module_instance;
        ModuleInstancePtr host_module;

        PluginHandle next_plugin_handle = 1;
        std::vector<ModuleInstance> loaded_modules;
        HostFunctionData host_fn_data;
    };

    PluginWindowPlacement from_c(miracle_placement_t placement, PluginHandle plugin_handle);

    std::mutex mutex_;
    std::unique_ptr<Self> self;
};
}
#else
#include "../miracle-plugin-rs/plugin.h"
#include "plugin_bridge.h"
#include <cstdint>
#include <mir/geometry/point.h>
#include <miral/toolkit_event.h>
#include <string>
namespace miracle
{
typedef uint32_t PluginHandle;
class PluginBridge;

struct PluginLoadResult
{
    bool success = false;
    PluginHandle handle = 0;
    std::string error;
};

class PluginManager
{
public:
    ~PluginManager() = default;
    void initialize(std::unique_ptr<PluginBridge>) {};
    PluginLoadResult load_wasm_module(std::string const&, std::string const& = "") { return PluginLoadResult {
        .success = false,
        .error = "Platform does not support plugins"
    }; }
    void unload_all() { }
    PluginHandle get_wasm_module(std::string const&) { return 0; }
    bool unload_wasm_module(PluginHandle) { return false; }
    std::optional<miracle_plugin_animation_frame_result_t> animate(
        AnimationData const&, float) { return std::nullopt; }
    void custom_animate(PluginHandle, uint32_t, float, float) { }
    std::optional<PluginWindowPlacement> place_new_window(
        miral::ApplicationInfo const&,
        miral::WindowSpecification const&,
        uint64_t) { return std::nullopt; }
    void window_deleted(miral::WindowInfo const&) { }
    void window_focused(miral::WindowInfo const&) { }
    void window_unfocused(miral::WindowInfo const&) { }
    void workspace_created(uint32_t) { }
    void workspace_removed(uint32_t) { }
    void workspace_focused(std::optional<uint32_t>, uint32_t) { }
    void workspace_area_changed(uint32_t) { }
    void window_workspace_changed(miral::WindowInfo const&, uint32_t) { }
    bool handle_keyboard_event(MirKeyboardEvent const&) { return false; }
    bool handle_pointer_event(MirPointerEvent const&) { return false; }
    PluginConfigData configure() { return PluginConfigData {}; }
};
}
#endif

#endif // MIRACLEWM_PLUGIN_MANAGER_H
