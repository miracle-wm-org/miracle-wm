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
#define MIR_LOG_COMPONENT "plugin_manager"
#include "plugin_manager.h"
#include "miracle/plugin.h"
#include <mir/log.h>

using namespace miracle;

#if ENABLE_PLUGIN_SYSTEM
namespace
{
WasmEdge_ConfigureContext* create_configure_context()
{
    auto const context = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(context, WasmEdge_HostRegistration_Wasi);
    return context;
}
}

PluginManager::PluginManager() :
    configure_context(create_configure_context()),
    store_context(WasmEdge_StoreCreate()),
    loader_context(WasmEdge_LoaderCreate(configure_context.get())),
    validator_context(WasmEdge_ValidatorCreate(configure_context.get())),
    executor_context(WasmEdge_ExecutorCreate(configure_context.get(), nullptr))
{
}

PluginLoadResult PluginManager::load_wasm_module(std::string const& path)
{
    // First, load the module.
    WasmEdge_ASTModuleContext* ast_module_context = nullptr;
    WasmEdge_Result r = WasmEdge_LoaderParseFromFile(loader_context.get(), &ast_module_context, path.c_str());
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Unable to parse wasm module from file %s: %s", path.c_str(), WasmEdge_ResultGetMessage(r));
        return PluginLoadResult {
            .success = false,
            .error = "Failed to parse wasm module."
        };
    }

    // Next, we validate the module.
    r = WasmEdge_ValidatorValidate(validator_context.get(), ast_module_context);
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Unable to validate wasm module from file %s: %s", path.c_str(), WasmEdge_ResultGetMessage(r));
        return PluginLoadResult {
            .success = false,
            .error = "Failed to validate wasm module."
        };
    }

    // Next, let's register the module context into the store by its name.
    auto const module_name = WasmEdge_StringCreateByCString("mod");
    WasmEdge_ModuleInstanceContext* module_context = nullptr;
    r = WasmEdge_ExecutorRegister(executor_context.get(), &module_context, store_context.get(), ast_module_context, module_name);
    WasmEdge_StringDelete(module_name);
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Unable to register wasm module from file %s: %s", path.c_str(), WasmEdge_ResultGetMessage(r));
        return PluginLoadResult {
            .success = false,
            .error = "Failed to register wasm module."
        };
    }

    // Now, we can inspect the exported functions.
    constexpr uint32_t BUF_LEN = 256;
    WasmEdge_String function_names[BUF_LEN];
    uint32_t num_functions = WasmEdge_ModuleInstanceListFunction(module_context, function_names, BUF_LEN);
    for (uint32_t i = 0; i < num_functions && i < BUF_LEN; i++)
    {
        char buf[BUF_LEN];
        uint32_t size = WasmEdge_StringCopy(function_names[i], buf, sizeof(buf));
        mir::log_info("Get exported function string length: %u, name: %s", size, buf);
    }

    // Finally, we will store the plugin for later.
    return PluginLoadResult {
        .success = true,
        .handle = next_plugin_handle++
    };
}

#endif
