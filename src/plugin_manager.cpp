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

#include "container.h"
#include "plugin_bridge.h"

#include <cstring>
#include <mir/log.h>

using namespace miracle;

#if FEATURE_PLUGIN_SYSTEM
namespace
{
// Helper to get memory instance from calling frame
WasmEdge_MemoryInstanceContext* get_memory_from_frame(
    WasmEdge_CallingFrameContext const* frame)
{
    auto const* module = WasmEdge_CallingFrameGetModuleInstance(frame);
    if (!module)
        return nullptr;

    auto const memory_name = WasmEdge_StringCreateByCString("memory");
    auto* memory = WasmEdge_ModuleInstanceFindMemory(module, memory_name);
    WasmEdge_StringDelete(memory_name);
    return memory;
}

WasmEdge_Result host_miracle_window_info_get_application(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_window_info_get_application: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_info_address = WasmEdge_ValueGetI64(params[0]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[2]);

    auto const application = bridge->application_from_window(window_info_address);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);

    // Get the application name from host memory
    char const* app_name = application.application_name;
    size_t const name_len = app_name ? std::strlen(app_name) : 0;

    // Check if name fits in buffer (need space for null terminator)
    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_window_info_get_application: buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI64(-1);
        return WasmEdge_Result_Success;
    }

    // Copy name to WASM linear memory
    if (app_name)
        std::memcpy(name_buf, app_name, name_len);
    name_buf[name_len] = '\0';

    // Return the internal ID (WASM already knows the name_buffer_ptr)
    returns[0] = WasmEdge_ValueGenI64(static_cast<int64_t>(application.internal));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_num_outputs(
    void* data,
    WasmEdge_CallingFrameContext const*,
    WasmEdge_Value const*,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    returns[0] = WasmEdge_ValueGenI32(bridge->num_outputs());
    return WasmEdge_Result_Success;
}

WasmEdge_Result return_output_internal(
    WasmEdge_MemoryInstanceContext* memory,
    int32_t const out_ptr,
    int32_t const name_buffer_ptr,
    int32_t const name_buffer_length,
    WasmEdge_Value* returns,
    PluginBridge::OutputResult const& output)
{
    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* output_buf = mem_base + out_ptr;
    std::memcpy(output_buf, &output.output, sizeof(output.output));

    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);

    // Get the output name from host memory
    char const* output_name = output.name.c_str();
    size_t const name_len = std::strlen(output_name);

    // Check if name fits in buffer (need space for null terminator)
    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_get_output_at: buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    // Copy name to WASM linear memory
    std::memcpy(name_buf, output_name, name_len);
    name_buf[name_len] = '\0';

    // Return success
    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_get_output_at(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_get_output_at: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint32_t const index = WasmEdge_ValueGetI32(params[0]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[3]);

    auto const output = bridge->output_at(index);
    return return_output_internal(memory, out_ptr, name_buffer_ptr, name_buffer_length, returns, output);
}

WasmEdge_Result host_miracle_workspace_get_output(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_get_output_at: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint64_t const workspace_address_ptr = WasmEdge_ValueGetI64(params[0]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[3]);

    auto const output = bridge->output_from_workspace(workspace_address_ptr);
    return return_output_internal(memory, out_ptr, name_buffer_ptr, name_buffer_length, returns, output);
}

WasmEdge_Result host_miracle_window_info_get_workspace(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_window_info_get_workspace: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint64_t const window_address = WasmEdge_ValueGetI64(params[0]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[3]);

    auto const workspace = bridge->workspace_from_window(window_address);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* workspace_buf = mem_base + out_ptr;
    std::memcpy(workspace_buf, &workspace.workspace, sizeof(workspace.workspace));

    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);

    // Get the output name from host memory
    char const* workspace_name = workspace.name.value_or("").c_str();
    size_t const name_len = std::strlen(workspace_name);

    // Check if name fits in buffer (need space for null terminator)
    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_window_info_get_workspace: buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    // Copy name to WASM linear memory
    std::memcpy(name_buf, workspace_name, name_len);
    name_buf[name_len] = '\0';

    // Return success
    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_workspace_get_tree(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_workspace_get_tree: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint64_t const workspace_address = WasmEdge_ValueGetI64(params[0]);
    uint32_t const index = WasmEdge_ValueGetI32(params[1]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[2]);

    auto const container = bridge->tree_at_index(workspace_address, index);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* workspace_buf = mem_base + out_ptr;
    std::memcpy(workspace_buf, &container, sizeof(container));

    // Return success
    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_container_get_child_at(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_container_get_child_at: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint64_t const container_address = WasmEdge_ValueGetI64(params[0]);
    uint32_t const index = WasmEdge_ValueGetI32(params[1]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[2]);

    auto const container = bridge->child_at(container_address, index);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* container_buf = mem_base + out_ptr;
    std::memcpy(container_buf, &container, sizeof(container));

    // Return success
    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_container_get_window(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_container_get_window: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint64_t const container_address = WasmEdge_ValueGetI64(params[0]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[3]);

    auto const window = bridge->get_window(container_address);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* workspace_buf = mem_base + out_ptr;
    std::memcpy(workspace_buf, &window.window_info, sizeof(window.window_info));

    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);
    char const* workspace_name = window.name.c_str();
    size_t const name_len = std::strlen(workspace_name);

    // Check if name fits in buffer (need space for null terminator)
    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_window_info_get_workspace: buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    // Copy name to WASM linear memory
    std::memcpy(name_buf, workspace_name, name_len);
    name_buf[name_len] = '\0';

    // Return success
    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_output_get_workspace(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_output_get_workspace: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint64_t const output_address = WasmEdge_ValueGetI64(params[0]);
    uint32_t const index = WasmEdge_ValueGetI32(params[1]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[3]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[4]);

    auto const workspace = bridge->workspace_on_output_at_index(output_address, index);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* workspace_buf = mem_base + out_ptr;
    std::memcpy(workspace_buf, &workspace.workspace, sizeof(workspace.workspace));

    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);

    char const* workspace_name = workspace.name.value_or("").c_str();
    size_t const name_len = std::strlen(workspace_name);

    // Check if name fits in buffer (need space for null terminator)
    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_window_info_get_workspace: buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    // Copy name to WASM linear memory
    std::memcpy(name_buf, workspace_name, name_len);
    name_buf[name_len] = '\0';

    // Return success
    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_ConfigureContext* create_configure_context()
{
    auto const context = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(context, WasmEdge_HostRegistration_Wasi);
    return context;
}

// Helper to create a function type
WasmEdge_FunctionTypeContext* create_func_type(
    std::vector<WasmEdge_ValType> const& params,
    std::vector<WasmEdge_ValType> const& returns)
{
    return WasmEdge_FunctionTypeCreate(
        params.data(), static_cast<uint32_t>(params.size()),
        returns.data(), static_cast<uint32_t>(returns.size()));
}

// Helper to add a host function to a module
void add_host_function(
    WasmEdge_ModuleInstanceContext* module,
    char const* name,
    WasmEdge_FunctionTypeContext* func_type,
    WasmEdge_HostFunc_t func,
    void* data)
{
    auto const func_name = WasmEdge_StringCreateByCString(name);
    auto* func_instance = WasmEdge_FunctionInstanceCreate(func_type, func, data, 0);
    WasmEdge_ModuleInstanceAddFunction(module, func_name, func_instance);
    WasmEdge_StringDelete(func_name);
    WasmEdge_FunctionTypeDelete(func_type);
}
}

PluginManager::Self::Self(std::unique_ptr<PluginBridge> bridge) :
    bridge(std::move(bridge)),
    configure_context(create_configure_context()),
    store_context(WasmEdge_StoreCreate()),
    loader_context(WasmEdge_LoaderCreate(configure_context.get())),
    validator_context(WasmEdge_ValidatorCreate(configure_context.get())),
    executor_context(WasmEdge_ExecutorCreate(configure_context.get(), nullptr))
{
    // Initialize and register WASI module
    auto* wasi_module = WasmEdge_ModuleInstanceCreateWASI(nullptr, 0, nullptr, 0, nullptr, 0);
    if (wasi_module)
    {
        // Log the WASI module name to verify it's correct
        auto const wasi_name = WasmEdge_ModuleInstanceGetModuleName(wasi_module);
        char wasi_name_buf[256];
        auto const wasi_name_len = WasmEdge_StringCopy(wasi_name, wasi_name_buf, sizeof(wasi_name_buf));
        mir::log_info("WASI module name: %.*s", static_cast<int>(wasi_name_len), wasi_name_buf);

        auto const r = WasmEdge_ExecutorRegisterImport(executor_context.get(), store_context.get(), wasi_module);
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to register WASI module: %s", WasmEdge_ResultGetMessage(r));
            WasmEdge_ModuleInstanceDelete(wasi_module);
        }
        else
        {
            wasi_module_instance.reset(wasi_module);
            mir::log_info("WASI module registered successfully");
        }
    }
    else
    {
        mir::log_error("Failed to create WASI module instance");
    }

    create_host_module();
}

PluginManager::Self::~Self() = default;

PluginManager::~PluginManager() = default;

void PluginManager::initialize(std::unique_ptr<PluginBridge> bridge)
{
    self = std::make_unique<Self>(std::move(bridge));
}

void PluginManager::Self::create_host_module()
{
    // Create the "env" module which is the standard import module name for C/C++ compiled WASM
    auto const module_name = WasmEdge_StringCreateByCString("env");
    auto* module = WasmEdge_ModuleInstanceCreate(module_name);
    WasmEdge_StringDelete(module_name);

    if (!module)
    {
        mir::log_error("Failed to create host module");
        return;
    }

    // Define WasmEdge value types
    WasmEdge_ValType const i32 = WasmEdge_ValTypeGenI32();
    WasmEdge_ValType const i64 = WasmEdge_ValTypeGenI64();

    add_host_function(module, "miracle_window_info_get_application",
        create_func_type({ i64, i32, i32 }, { i64 }),
        host_miracle_window_info_get_application, bridge.get());

    add_host_function(module, "miracle_num_outputs",
        create_func_type({}, { i32 }),
        host_miracle_num_outputs, bridge.get());

    add_host_function(module, "miracle_get_output_at",
        create_func_type({ i32, i32, i32, i32 }, { i32 }),
        host_miracle_get_output_at, bridge.get());

    add_host_function(module, "miracle_window_info_get_workspace",
        create_func_type({ i64, i32, i32, i32 }, { i32 }),
        host_miracle_window_info_get_workspace, bridge.get());

    add_host_function(module, "miracle_workspace_get_output",
        create_func_type({ i64, i32, i32, i32 }, { i32 }),
        host_miracle_workspace_get_output, bridge.get());

    add_host_function(module, "miracle_workspace_get_tree",
        create_func_type({ i64, i32, i32 }, { i32 }),
        host_miracle_workspace_get_tree, bridge.get());

    add_host_function(module, "miracle_container_get_child_at",
        create_func_type({ i64, i32, i32 }, { i32 }),
        host_miracle_container_get_child_at, bridge.get());

    add_host_function(module, "miracle_container_get_window",
        create_func_type({ i64, i32, i32, i32 }, { i32 }),
        host_miracle_container_get_window, bridge.get());

    add_host_function(module, "miracle_output_get_workspace",
        create_func_type({ i64, i32, i32, i32, i32 }, { i32 }),
        host_miracle_output_get_workspace, bridge.get());

    // Register the host module with the executor
    auto const r = WasmEdge_ExecutorRegisterImport(executor_context.get(), store_context.get(), module);
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to register host module: %s", WasmEdge_ResultGetMessage(r));
        WasmEdge_ModuleInstanceDelete(module);
        return;
    }

    host_module.reset(module);
    mir::log_info("Host module 'env' registered with %d functions", 8);
}

PluginLoadResult PluginManager::load_wasm_module(std::string const& path, std::string const& name)
{
    std::lock_guard lock(mutex);
    auto const erased = std::erase_if(self->loaded_modules, [&name](auto const& module)
    {
        return module.name == name;
    });
    if (erased > 0)
        mir::log_info("Module with name %s was already loaded, unloading previous instance.", name.c_str());

    // First, load the module.
    WasmEdge_ASTModuleContext* ast_module_context = nullptr;
    WasmEdge_Result r = WasmEdge_LoaderParseFromFile(self->loader_context.get(), &ast_module_context, path.c_str());
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Unable to parse wasm module from file %s: %s", path.c_str(), WasmEdge_ResultGetMessage(r));
        return PluginLoadResult {
            .success = false,
            .error = "Failed to parse wasm module."
        };
    }

    // Next, we validate the module.
    r = WasmEdge_ValidatorValidate(self->validator_context.get(), ast_module_context);
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
    r = WasmEdge_ExecutorRegister(self->executor_context.get(), &module_context, self->store_context.get(), ast_module_context, module_name);
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
    std::bitset<static_cast<uint8_t>(Self::ProvidedFunction::max)> provided_functions;
    for (uint32_t i = 0; i < num_functions && i < BUF_LEN; i++)
    {
        char buf[BUF_LEN];
        auto const size = WasmEdge_StringCopy(function_names[i], buf, sizeof(buf));
        if (size == 0)
            continue;

        if (std::string_view { buf, size } == "add_points")
        {
            provided_functions.set(static_cast<size_t>(Self::ProvidedFunction::add_points), true);
            mir::log_info("Module provides 'add_points' function.");
        }
        else if (std::string_view { buf, size } == "animate")
        {
            provided_functions.set(static_cast<size_t>(Self::ProvidedFunction::animate), true);
            mir::log_info("Module provides 'animate' function.");
        }
        else if (std::string_view { buf, size } == "place_new_window")
        {
            provided_functions.set(static_cast<size_t>(Self::ProvidedFunction::place_new_window), true);
            mir::log_info("Module provides 'place_new_window' function.");
        }
    }

    // Finally, we will store the plugin for later.
    auto const handle = self->next_plugin_handle++;
    self->loaded_modules.push_back(Self::ModuleInstance {
        Self::ModuleInstancePtr { module_context },
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
    for (auto const& module : self->loaded_modules)
    {
        if (module.name == name)
            return module.handle;
    }

    return 0;
}

bool PluginManager::unload_wasm_module(PluginHandle handle)
{
    std::lock_guard lock(mutex);
    auto const erased = std::erase_if(self->loaded_modules, [handle](auto const& module)
    {
        return module.handle == handle;
    });
    return erased > 0;
}

void PluginManager::unload_all()
{
    std::lock_guard lock(mutex);
    self->loaded_modules.clear();
}

mir::geometry::Point PluginManager::add_points(mir::geometry::Point first, mir::geometry::Point second)
{
    std::lock_guard lock(mutex);
    for (auto const& module : self->loaded_modules)
    {
        if (!module.provided_functions.test(static_cast<size_t>(Self::ProvidedFunction::add_points)))
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
            self->executor_context.get(),
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
    Self::ModuleInstance* target_module = nullptr;
    for (auto& module : self->loaded_modules)
    {
        if (module.handle == handle)
        {
            target_module = &module;
            break;
        }
    }

    // If module not found or doesn't provide animate function, return completed
    if (target_module == nullptr || !target_module->provided_functions.test(static_cast<size_t>(Self::ProvidedFunction::animate)))
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
        self->executor_context.get(),
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

PluginWindowPlacement PluginManager::place_new_window(
    PluginHandle handle,
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification const& spec)
{
    std::lock_guard lock(mutex);
    auto const bridge_handle = self->bridge->new_window_info(app_info, spec);
    auto const window_info_t = bridge_handle.get();
    // Find the module with the given handle
    Self::ModuleInstance* target_module = nullptr;
    for (auto& module : self->loaded_modules)
    {
        if (module.handle == handle)
        {
            target_module = &module;
            break;
        }
    }

    // If module not found or doesn't provide place_new_window function, return unset placement
    if (target_module == nullptr || !target_module->provided_functions.test(static_cast<size_t>(Self::ProvidedFunction::place_new_window)))
    {
        mir::log_warning("Module with handle %u not found or doesn't provide 'place_new_window' function.", handle);
        return PluginWindowPlacement {
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
        return PluginWindowPlacement {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Allocate memory for the structs in WASM linear memory
    uint32_t constexpr result_ptr = 0;
    uint32_t constexpr window_info_ptr = sizeof(miracle_window_info_t);

    // Write window_info to WASM memory
    uint8_t window_info_buffer[sizeof(miracle_window_info_t)];
    std::memcpy(window_info_buffer, &window_info_t, sizeof(window_info_t));
    auto r = WasmEdge_MemoryInstanceSetData(
        memory_context,
        window_info_buffer,
        window_info_ptr,
        sizeof(window_info_buffer));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to write window_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return PluginWindowPlacement {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Prepare the parameters
    // param[0]: pointer to result location
    // param[1]: pointer to window_info
    WasmEdge_Value params[2];
    params[0] = WasmEdge_ValueGenI32(result_ptr);
    params[1] = WasmEdge_ValueGenI32(window_info_ptr);

    // Call the function
    auto const func_name = WasmEdge_StringCreateByCString("place_new_window");
    auto const func_context = WasmEdge_ModuleInstanceFindFunction(target_module->module_context.get(), func_name);
    WasmEdge_StringDelete(func_name);

    if (func_context == nullptr)
    {
        mir::log_error("Function 'place_new_window' not found in module.");
        return PluginWindowPlacement {
            .strategy = miracle_window_management_strategy_system
        };
    }

    r = WasmEdge_ExecutorInvoke(
        self->executor_context.get(),
        func_context,
        params,
        2,
        nullptr,
        0);

    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to invoke 'place_new_window' function: %s", WasmEdge_ResultGetMessage(r));
        return PluginWindowPlacement {
            .strategy = miracle_window_management_strategy_system
        };
    }

    // Read the result from WASM memory
    miracle_placement_t result;
    r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(&result), result_ptr, sizeof(result));
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
        return PluginWindowPlacement {
            .strategy = miracle_window_management_strategy_system
        };
    }

    return from_c(result);
}

PluginWindowPlacement PluginManager::from_c(miracle_placement_t placement)
{
    PluginWindowPlacement result;
    result.strategy = static_cast<miracle_window_management_strategy_t>(placement.strategy);
    switch (result.strategy)
    {
    case miracle_window_management_strategy_tiled:
    {
        result.tiled.container = self->bridge->resolve_container(placement.tiled_placement.parent_internal);
        result.tiled.index = placement.tiled_placement.index;
        result.tiled.scheme = from_layout(placement.tiled_placement.layout_scheme);
        break;
    }
    case miracle_window_management_strategy_freestyle:
    {
        result.freestyle.rectangle = geom::Rectangle(
            from_point(placement.freestyle_placement.top_left),
            from_size(placement.freestyle_placement.size));
        result.freestyle.layer = static_cast<MirDepthLayer>(placement.freestyle_placement.depth_layer);
        result.freestyle.workspace = self->bridge->resolve_workspace(placement.freestyle_placement.workspace_internal);
        break;
    }
    default:
        break;
    }
    return result;
}

#endif
