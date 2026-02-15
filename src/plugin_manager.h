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
#include "layout_scheme.h"
#include "miracle/plugin.h"
#include <mir/geometry/rectangle.h>

namespace miracle
{
class Container;
class WorkspaceInterface;

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
        mir::geometry::Rectangle rectangle;
        MirDepthLayer layer;
        WorkspaceInterface* workspace;
    };

    miracle_window_management_strategy_t strategy = miracle_window_management_strategy_system;
    TiledPlacement tiled;
    FreestylePlacement freestyle;
};
}

#if FEATURE_PLUGIN_SYSTEM
#include <memory>
#include <mir/geometry/point.h>
#include <mutex>
#include <vector>
#include <wasmedge/wasmedge.h>

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
    /// \returns a load result.
    PluginLoadResult load_wasm_module(std::string const& path);

    /// Get the WebAssembly module associated with \p name.
    ///
    /// \param name The name of the module.
    /// \returns The plugin handle, or 0 if not found.
    PluginHandle get_wasm_module(std::string const& name);

    /// Attempt to unload the WebAssembly module associated with \p handle.
    ///
    /// \returns `true` if the module was unloaded, `false` otherwise.
    bool unload_wasm_module(PluginHandle handle);

    /// Unload all loaded WebAssembly modules.
    void unload_all();

    /// Animate a frame via the plugin system.
    ///
    /// \param frame_data The frame data to animate.
    /// \returns The result of the animation frame, or none if none is set.
    std::optional<miracle_plugin_animation_frame_result_t> animate(
        miracle_plugin_animation_frame_data_t const& frame_data);

    /// Try to place a new window using a plugin.
    ///
    /// \param app_info The application info.
    /// \param spec The window specification.
    /// \returns the placement
    std::optional<PluginWindowPlacement> place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& spec);

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
            ModuleInstancePtr module_context;
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
    };

    PluginWindowPlacement from_c(miracle_placement_t placement);

    std::mutex mutex;
    std::unique_ptr<Self> self;
};
}
#else
#include "miracle/plugin.h"
#include "plugin_bridge.h"
#include <cstdint>
#include <mir/geometry/point.h>
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
    PluginLoadResult load_wasm_module(std::string const&, std::string const&) { return PluginLoadResult {
        .success = false,
        .error = "Platform does not support plugins"
    }; }
    void unload_all() { }
    PluginHandle get_wasm_module(std::string const&) { return 0; }
    bool unload_wasm_module(PluginHandle) { return false; }
    std::optional<miracle_plugin_animation_frame_result_t> animate_frame(
        miracle_plugin_animation_frame_data_t const&) { return std::nullopt; }
    std::optional<PluginWindowPlacement> place_new_window(
        miral::ApplicationInfo const&,
        miral::WindowSpecification const&) { return std::nullopt; }
};
}
#endif

#endif // MIRACLEWM_PLUGIN_MANAGER_H
