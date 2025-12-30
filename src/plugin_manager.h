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

#if FEATURE_PLUGIN_SYSTEM
#include "miracle/plugin.h"
#include <bitset>
#include <memory>
#include <mir/geometry/point.h>
#include <mutex>
#include <vector>
#include <wasmedge/wasmedge.h>

namespace miracle
{
typedef uint32_t PluginHandle;

struct PluginLoadResult
{
    bool success = false;
    PluginHandle handle = 0;
    std::string error;
};

class PluginManager
{
public:
    PluginManager();

    /// Load a WebAssembly module from the provided \p path.
    ///
    /// Callers may use the #PluginLoadResult::handle to unload the module later.
    ///
    /// \param path The filesystem path to the WebAssembly module.
    /// \param name The name of the module.
    /// \returns a load result.
    PluginLoadResult load_wasm_module(std::string const& path, std::string const& name);

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

    /// Example function that adds two points together.
    ///
    /// This method will call into a WebAssembly function to perform the addition.
    ///
    /// \param first The first point to add.
    /// \param second The second point to add.
    /// \returns The sum of the two points.
    mir::geometry::Point add_points(mir::geometry::Point first, mir::geometry::Point second);

    /// Animate a frame using the provided plugin handle and frame data.
    ///
    /// If \p handle does not correspond to a loaded plugin, the function will
    /// return a resulting indicating that the animation is finished.
    ////
    /// \param handle The plugin handle to use for animation.
    /// \param frame_data The frame data to animate.
    /// \returns The result of the animation frame.
    miracle_plugin_animation_frame_result_t animate_frame(
        PluginHandle handle,
        miracle_plugin_animation_frame_data_t const& frame_data);

private:
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

    enum class ProvidedFunction : std::uint8_t
    {
        add_points,
        animate,
        max
    };

    struct ModuleInstance
    {
        ModuleInstancePtr module_context;
        std::bitset<static_cast<uint8_t>(ProvidedFunction::max)> provided_functions;
        PluginHandle handle;
        std::string name;
    };

    std::mutex mutex;
    ConfigurePtr configure_context;
    StorePtr store_context;
    LoaderPtr loader_context;
    ValidtorPtr validator_context;
    ExecutorPtr executor_context;
    PluginHandle next_plugin_handle = 1;
    std::vector<ModuleInstance> loaded_modules;
};
}
#else
#include "miracle/plugin.h"
#include <cstdint>
#include <mir/geometry/point.h>
#include <string>
namespace miracle
{
typedef uint32_t PluginHandle;

struct PluginLoadResult
{
    bool success = false;
    PluginHandle handle = 0;
    std::string error;
};

class PluginManager
{
public:
    PluginManager() = default;
    PluginLoadResult load_wasm_module(std::string const&, std::string const&) { return PluginLoadResult {
        .success = false,
        .error = "Platform does not support plugins"
    }; }
    void unload_all() { }
    PluginHandle get_wasm_module(std::string const&) { return 0; }
    bool unload_wasm_module(PluginHandle) { return false; }
    mir::geometry::Point add_points(mir::geometry::Point, mir::geometry::Point) { return mir::geometry::Point {}; }
    miracle_plugin_animation_frame_result_t animate_frame(
        PluginHandle,
        miracle_plugin_animation_frame_data_t const&) { return miracle_plugin_animation_frame_result_t {}; }
};
}
#endif

#endif // MIRACLEWM_PLUGIN_MANAGER_H
