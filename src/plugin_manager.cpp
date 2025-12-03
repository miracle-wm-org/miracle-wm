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
    auto const num_functions = WasmEdge_ModuleInstanceListFunction(module_context, function_names, BUF_LEN);
    std::bitset<sizeof(uint8_t)> provided_functions;
    for (uint32_t i = 0; i < num_functions && i < BUF_LEN; i++)
    {
        char buf[BUF_LEN];
        auto const size = WasmEdge_StringCopy(function_names[i], buf, sizeof(buf));
        if (size == 0)
            continue;

        if (std::string_view{buf, size} == "add_points")
        {
            provided_functions.set(static_cast<size_t>(ProvidedFunction::add_points), true);
            mir::log_info("Module provides 'add_points' function.");
        }
    }

    // Finally, we will store the plugin for later.
    loaded_modules.push_back(ModuleInstance{
        ModuleInstancePtr{module_context},
        provided_functions
    });

    return PluginLoadResult {
        .success = true,
        .handle = next_plugin_handle++
    };
}

mir::geometry::Point PluginManager::add_points(mir::geometry::Point first, mir::geometry::Point second)
{
    for (auto const& module : loaded_modules)
    {
        if (!module.provided_functions.test(static_cast<size_t>(ProvidedFunction::add_points)))
            continue;

        // Get the memory context from the module instance
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        // Allocate memory for the structs in WASM linear memory
        // Each Point struct is 16 bytes (2 x f64)
        // We need: result_ptr (16 bytes), first_ptr (16 bytes), second_ptr (16 bytes)
        uint32_t result_ptr = 0;  // Offset 0
        uint32_t first_ptr = 16;  // Offset 16
        uint32_t second_ptr = 32; // Offset 32

        // Write first Point to WASM memory
        double first_data[2] = {static_cast<double>(first.x.as_int()), static_cast<double>(first.y.as_int())};
        auto r = WasmEdge_MemoryInstanceSetData(memory_context, reinterpret_cast<uint8_t*>(first_data), first_ptr, sizeof(first_data));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write first point to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        // Write second Point to WASM memory
        double second_data[2] = {static_cast<double>(second.x.as_int()), static_cast<double>(second.y.as_int())};
        r = WasmEdge_MemoryInstanceSetData(memory_context, reinterpret_cast<uint8_t*>(second_data), second_ptr, sizeof(second_data));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write second point to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        // Prepare the parameters as i32 pointers (C ABI calling convention)
        // param[0]: pointer to result location
        // param[1]: pointer to first Point
        // param[2]: pointer to second Point
        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(result_ptr);
        params[1] = WasmEdge_ValueGenI32(first_ptr);
        params[2] = WasmEdge_ValueGenI32(second_ptr);

        // Call the function (no return value, result written to memory)
        auto const func_name = WasmEdge_StringCreateByCString("add_points");
        auto const func_context =
            WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);
        if (func_context == nullptr)
        {
            mir::log_error("Function 'add_points' not found in module.");
            continue;
        }

        r = WasmEdge_ExecutorInvoke(
            executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'add_points' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        // Read the result from WASM memory
        double result_data[2];
        r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(result_data), result_ptr, sizeof(result_data));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        mir::log_info("Successfully added points using 'add_points' function: (%f, %f)", result_data[0], result_data[1]);
        return mir::geometry::Point{static_cast<int>(result_data[0]), static_cast<int>(result_data[1])};
    }

    // Fallback: return a default point if no module provides add_points
    return mir::geometry::Point{0, 0};
}


#endif
