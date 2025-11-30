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
#include <wasmedge/wasmedge.h>
namespace miracle
{
class PluginManager
{
public:
    PluginManager();

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

    ConfigurePtr configure_context;
    StorePtr store_context;
    VMPtr vm_context;
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