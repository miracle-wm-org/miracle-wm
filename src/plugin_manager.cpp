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
#include <mir/log.h>

using namespace miracle;

#if FEATURE_PLUGIN_SYSTEM
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

PluginLoadResult PluginManager::load_wasm_module(std::string const& path, std::string const& name)
{
    std::lock_guard lock(mutex);
    auto const erased = std::erase_if(loaded_modules, [&name](auto const& module)
    {
        return module.name == name;
    });
    if (erased > 0)
        mir::log_info("Module with name %s was already loaded, unloading previous instance.", name.c_str());

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
    auto const module_name = WasmEdge_StringCreateByCString(name.c_str());
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
    std::bitset<static_cast<uint8_t>(ProvidedFunction::max)> provided_functions;
    for (uint32_t i = 0; i < num_functions && i < BUF_LEN; i++)
    {
        char buf[BUF_LEN];
        auto const size = WasmEdge_StringCopy(function_names[i], buf, sizeof(buf));
        if (size == 0)
            continue;

        if (std::string_view { buf, size } == "add_points")
        {
            provided_functions.set(static_cast<size_t>(ProvidedFunction::add_points), true);
            mir::log_info("Module provides 'add_points' function.");
        }
        else if (std::string_view { buf, size } == "animate")
        {
            provided_functions.set(static_cast<size_t>(ProvidedFunction::animate), true);
            mir::log_info("Module provides 'animate' function.");
        }
        else if (std::string_view { buf, size } == "place_new_window")
        {
            provided_functions.set(static_cast<size_t>(ProvidedFunction::place_new_window), true);
            mir::log_info("Module provides 'place_new_window' function.");
        }
    }

    // Finally, we will store the plugin for later.
    auto const handle = next_plugin_handle++;
    loaded_modules.push_back(ModuleInstance {
        ModuleInstancePtr { module_context },
        provided_functions,
        handle,
        name });

    return PluginLoadResult {
        .success = true,
        .handle = handle
    };
}

PluginHandle PluginManager::get_wasm_module(std::string const& name)
{
    std::lock_guard lock(mutex);
    for (auto const& module : loaded_modules)
    {
        if (module.name == name)
            return module.handle;
    }

    return 0;
}

bool PluginManager::unload_wasm_module(PluginHandle handle)
{
    std::lock_guard lock(mutex);
    auto const erased = std::erase_if(loaded_modules, [handle](auto const& module)
    {
        return module.handle == handle;
    });
    return erased > 0;
}

void PluginManager::unload_all()
{
    std::lock_guard lock(mutex);
    loaded_modules.clear();
}

mir::geometry::Point PluginManager::add_points(mir::geometry::Point first, mir::geometry::Point second)
{
    std::lock_guard lock(mutex);
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
        // We need: result_ptr, first_ptr, and second_ptr.
        uint32_t constexpr result_ptr = 0; // Offset 0
        uint32_t constexpr first_ptr = sizeof(miracle_point_t);
        uint32_t constexpr second_ptr = 2 * sizeof(miracle_point_t);

        // Write first Point to WASM memory
        int32_t first_data[2] = { first.x.as_int(), first.y.as_int() };
        auto r = WasmEdge_MemoryInstanceSetData(memory_context, reinterpret_cast<uint8_t*>(first_data), first_ptr, sizeof(first_data));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write first point to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        // Write second Point to WASM memory
        int32_t second_data[2] = { second.x.as_int(), second.y.as_int() };
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
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
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
        int32_t result_data[2];
        r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(result_data), result_ptr, sizeof(result_data));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        mir::log_info("Successfully added points using 'add_points' function: (%d, %d)", result_data[0], result_data[1]);
        return mir::geometry::Point { result_data[0], result_data[1] };
    }

    // Fallback: return a default point if no module provides add_points
    return mir::geometry::Point { 0, 0 };
}

miracle_plugin_animation_frame_result_t PluginManager::animate_frame(
    PluginHandle handle,
    miracle_plugin_animation_frame_data_t const& frame_data)
{
    std::lock_guard lock(mutex);
    // Find the module with the given handle
    ModuleInstance* target_module = nullptr;
    for (auto& module : loaded_modules)
    {
        if (module.handle == handle)
        {
            target_module = &module;
            break;
        }
    }

    // If module not found or doesn't provide animate function, return completed
    if (target_module == nullptr || !target_module->provided_functions.test(static_cast<size_t>(ProvidedFunction::animate)))
    {
        mir::log_warning("Module with handle %u not found or doesn't provide 'animate' function.", handle);
        return miracle_plugin_animation_frame_result_t {
            .completed = 1,
            .has_area = 0,
            .has_transform = 0,
            .has_opacity = 0
        };
    }

    // Get the memory context from the module instance
    auto const memory_name = WasmEdge_StringCreateByCString("memory");
    auto const memory_context = WasmEdge_ModuleInstanceFindMemory(target_module->module_context.get(), memory_name);
    WasmEdge_StringDelete(memory_name);

    if (memory_context == nullptr)
    {
        mir::log_error("Memory not found in module.");
        return miracle_plugin_animation_frame_result_t {
            .completed = 1,
            .has_area = 0,
            .has_transform = 0,
            .has_opacity = 0
        };
    }

    // Allocate memory for the structs in WASM linear memory
    uint32_t constexpr result_ptr = 0;
    uint32_t constexpr frame_data_ptr = sizeof(miracle_plugin_animation_frame_result_t);

    // Write frame_data to WASM memory
    uint8_t frame_data_buffer[sizeof(miracle_plugin_animation_frame_data_t)];
    std::memcpy(frame_data_buffer, &frame_data, sizeof(frame_data));
    auto r = WasmEdge_MemoryInstanceSetData(
        memory_context,
        frame_data_buffer,
        frame_data_ptr,
        sizeof(frame_data_buffer));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to write frame_data to WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return miracle_plugin_animation_frame_result_t {
            .completed = 1,
            .has_area = 0,
            .has_transform = 0,
            .has_opacity = 0
        };
    }

    // Prepare the parameters
    // param[0]: pointer to result location
    // param[1]: pointer to frame_data
    WasmEdge_Value params[2];
    params[0] = WasmEdge_ValueGenI32(result_ptr);
    params[1] = WasmEdge_ValueGenI32(frame_data_ptr);

    // Call the function
    auto const func_name = WasmEdge_StringCreateByCString("animate");
    auto const func_context = WasmEdge_ModuleInstanceFindFunction(target_module->module_context.get(), func_name);
    WasmEdge_StringDelete(func_name);

    if (func_context == nullptr)
    {
        mir::log_error("Function 'animate' not found in module.");
        return miracle_plugin_animation_frame_result_t {
            .completed = 1,
            .has_area = 0,
            .has_transform = 0,
            .has_opacity = 0
        };
    }

    r = WasmEdge_ExecutorInvoke(
        executor_context.get(),
        func_context,
        params,
        2,
        nullptr,
        0);

    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to invoke 'animate' function: %s", WasmEdge_ResultGetMessage(r));
        return miracle_plugin_animation_frame_result_t {
            .completed = true,
            .has_area = false,
            .has_transform = false,
            .has_opacity = false
        };
    }

    // Read the result from WASM memory
    miracle_plugin_animation_frame_result_t result;
    r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(&result), result_ptr, sizeof(result));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return miracle_plugin_animation_frame_result_t {
            .completed = 1,
            .has_area = 0,
            .has_transform = 0,
            .has_opacity = 0
        };
    }

    mir::log_info("Successfully animated frame: completed=%d, has_area=%d, has_transform=%d, has_opacity=%d",
        result.completed, result.has_area, result.has_transform, result.has_opacity);

    return result;
}

miracle_placement_t PluginManager::place_new_window(
    PluginHandle handle,
    miracle_context_t const& context,
    miracle_window_info_t const& window_info)
{
    std::lock_guard lock(mutex);
    // Find the module with the given handle
    ModuleInstance* target_module = nullptr;
    for (auto& module : loaded_modules)
    {
        if (module.handle == handle)
        {
            target_module = &module;
            break;
        }
    }

    // If module not found or doesn't provide place_new_window function, return unset placement
    if (target_module == nullptr || !target_module->provided_functions.test(static_cast<size_t>(ProvidedFunction::place_new_window)))
    {
        mir::log_warning("Module with handle %u not found or doesn't provide 'place_new_window' function.", handle);
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Get the memory context from the module instance
    auto const memory_name = WasmEdge_StringCreateByCString("memory");
    auto const memory_context = WasmEdge_ModuleInstanceFindMemory(target_module->module_context.get(), memory_name);
    WasmEdge_StringDelete(memory_name);

    if (memory_context == nullptr)
    {
        mir::log_error("Memory not found in module.");
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Allocate memory for the structs in WASM linear memory
    uint32_t constexpr result_ptr = 0;
    uint32_t constexpr context_ptr = sizeof(miracle_placement_t);
    uint32_t constexpr window_info_ptr = context_ptr + sizeof(miracle_context_t);

    // Write context to WASM memory
    uint8_t context_buffer[sizeof(miracle_context_t)];
    std::memcpy(context_buffer, &context, sizeof(context));
    auto r = WasmEdge_MemoryInstanceSetData(
        memory_context,
        context_buffer,
        context_ptr,
        sizeof(context_buffer));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to write context to WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Write window_info to WASM memory
    uint8_t window_info_buffer[sizeof(miracle_window_info_t)];
    std::memcpy(window_info_buffer, &window_info, sizeof(window_info));
    r = WasmEdge_MemoryInstanceSetData(
        memory_context,
        window_info_buffer,
        window_info_ptr,
        sizeof(window_info_buffer));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to write window_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Prepare the parameters
    // param[0]: pointer to result location
    // param[1]: pointer to context
    // param[2]: pointer to window_info
    WasmEdge_Value params[3];
    params[0] = WasmEdge_ValueGenI32(result_ptr);
    params[1] = WasmEdge_ValueGenI32(context_ptr);
    params[2] = WasmEdge_ValueGenI32(window_info_ptr);

    // Call the function
    auto const func_name = WasmEdge_StringCreateByCString("place_new_window");
    auto const func_context = WasmEdge_ModuleInstanceFindFunction(target_module->module_context.get(), func_name);
    WasmEdge_StringDelete(func_name);

    if (func_context == nullptr)
    {
        mir::log_error("Function 'place_new_window' not found in module.");
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
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
        mir::log_error("Failed to invoke 'place_new_window' function: %s", WasmEdge_ResultGetMessage(r));
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Read the result from WASM memory
    miracle_placement_t result;
    r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(&result), result_ptr, sizeof(result));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return miracle_placement_t {
            .strategy = miracle_window_management_strategy_system
        };
    }

    return result;
}

#endif
