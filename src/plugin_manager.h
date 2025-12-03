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

#if ENABLE_PLUGIN_SYSTEM
#include <memory>
#include <mir/geometry/point.h>
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
    /// \returns a load result.
    PluginLoadResult load_wasm_module(std::string const& path);

    /// Attempt to unload the WebAssembly module associated with \p handle.
    ///
    /// \returns `true` if the module was unloaded, `false` otherwise.
    /// bool unload_wasm_module(PluginHandle handle);

    /// Example function that adds two points together.
    ///
    /// This method will call into a WebAssembly function to perform the addition.
    ///
    /// \param first The first point to add.
    /// \param second The second point to add.
    /// \returns The sum of the two points.
    /// mir::geometry::Point add_points(mir::geometry::Point first, mir::geometry::Point second);

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

    ConfigurePtr configure_context;
    StorePtr store_context;
    LoaderPtr loader_context;
    ValidtorPtr validator_context;
    ExecutorPtr executor_context;

    PluginHandle next_plugin_handle = 1;
};
}
#else
namespace miracle
{
class PluginManager
{
public:
    PluginManager() = default;
};
}
#endif

#endif // MIRACLEWM_PLUGIN_MANAGER_H