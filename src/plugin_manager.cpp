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

WasmEdge_Result host_miracle_request_workspace(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_request_workspace: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    int32_t const has_number = WasmEdge_ValueGetI32(params[0]);
    int32_t const number = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_in_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_in_len = WasmEdge_ValueGetI32(params[3]);
    int32_t const out_workspace_ptr = WasmEdge_ValueGetI32(params[4]);
    int32_t const out_name_buf_ptr = WasmEdge_ValueGetI32(params[5]);
    int32_t const out_name_buf_len = WasmEdge_ValueGetI32(params[6]);
    int32_t const focus_workspace = WasmEdge_ValueGetI32(params[7]);

    std::optional<int> num;
    if (has_number)
        num = number;

    std::optional<std::string> name;
    if (name_in_ptr != 0 && name_in_len > 0)
    {
        uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
        char const* name_str = reinterpret_cast<char const*>(mem_base + name_in_ptr);
        name = std::string(name_str, name_in_len);
    }

    auto const workspace = bridge->request_workspace(num, name, focus_workspace != 0);
    if (!workspace.workspace.is_set)
    {
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* workspace_buf = mem_base + out_workspace_ptr;
    std::memcpy(workspace_buf, &workspace.workspace, sizeof(workspace.workspace));

    char* name_buf = reinterpret_cast<char*>(mem_base + out_name_buf_ptr);
    char const* workspace_name = workspace.name.value_or("").c_str();
    size_t const name_len = std::strlen(workspace_name);

    if (name_len + 1 > static_cast<size_t>(out_name_buf_len))
    {
        mir::log_error("host_miracle_request_workspace: name buffer too small (%zu > %d)",
            name_len + 1, out_name_buf_len);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    std::memcpy(name_buf, workspace_name, name_len);
    name_buf[name_len] = '\0';

    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_get_active_workspace(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_get_active_workspace: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[0]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[2]);

    auto const workspace = bridge->active_workspace();
    if (!workspace.workspace.is_set)
    {
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* workspace_buf = mem_base + out_ptr;
    std::memcpy(workspace_buf, &workspace.workspace, sizeof(workspace.workspace));

    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);
    char const* workspace_name = workspace.name.value_or("").c_str();
    size_t const name_len = std::strlen(workspace_name);

    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_get_active_workspace: name buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    std::memcpy(name_buf, workspace_name, name_len);
    name_buf[name_len] = '\0';

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

WasmEdge_Result host_miracle_num_managed_windows(
    void* data,
    WasmEdge_CallingFrameContext const*,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    uint32_t const plugin_handle = WasmEdge_ValueGetI32(params[0]);
    returns[0] = WasmEdge_ValueGenI32(bridge->num_managed_windows(plugin_handle));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_get_managed_window_at(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_get_managed_window_at: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    uint32_t const plugin_handle = WasmEdge_ValueGetI32(params[0]);
    uint32_t const index = WasmEdge_ValueGetI32(params[1]);
    int32_t const out_ptr = WasmEdge_ValueGetI32(params[2]);
    int32_t const name_buffer_ptr = WasmEdge_ValueGetI32(params[3]);
    int32_t const name_buffer_length = WasmEdge_ValueGetI32(params[4]);

    auto const window = bridge->get_managed_window_at(plugin_handle, index);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    uint8_t* window_buf = mem_base + out_ptr;
    std::memcpy(window_buf, &window.window_info, sizeof(window.window_info));

    char* name_buf = reinterpret_cast<char*>(mem_base + name_buffer_ptr);
    char const* window_name = window.name.c_str();
    size_t const name_len = std::strlen(window_name);

    if (name_len + 1 > static_cast<size_t>(name_buffer_length))
    {
        mir::log_error("host_miracle_get_managed_window_at: buffer too small (%zu > %d)",
            name_len + 1, name_buffer_length);
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }

    std::memcpy(name_buf, window_name, name_len);
    name_buf[name_len] = '\0';

    returns[0] = WasmEdge_ValueGenI32(0);
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_window_set_state(
    void* data,
    WasmEdge_CallingFrameContext const*,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_internal = WasmEdge_ValueGetI64(params[0]);
    int32_t const state = WasmEdge_ValueGetI32(params[1]);
    returns[0] = WasmEdge_ValueGenI32(bridge->window_set_state(static_cast<uint64_t>(window_internal), state));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_window_set_workspace(
    void* data,
    WasmEdge_CallingFrameContext const*,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_internal = WasmEdge_ValueGetI64(params[0]);
    int64_t const workspace_internal = WasmEdge_ValueGetI64(params[1]);
    returns[0] = WasmEdge_ValueGenI32(bridge->window_set_workspace(
        static_cast<uint64_t>(window_internal), static_cast<uint64_t>(workspace_internal)));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_window_set_rectangle(
    void* data,
    WasmEdge_CallingFrameContext const*,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_internal = WasmEdge_ValueGetI64(params[0]);
    int32_t const x = WasmEdge_ValueGetI32(params[1]);
    int32_t const y = WasmEdge_ValueGetI32(params[2]);
    int32_t const width = WasmEdge_ValueGetI32(params[3]);
    int32_t const height = WasmEdge_ValueGetI32(params[4]);
    int32_t const animate = WasmEdge_ValueGetI32(params[5]);
    returns[0] = WasmEdge_ValueGenI32(bridge->window_set_rectangle(
        static_cast<uint64_t>(window_internal), x, y, width, height, animate != 0));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_window_set_transform(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_window_set_transform: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_internal = WasmEdge_ValueGetI64(params[0]);
    int32_t const transform_ptr = WasmEdge_ValueGetI32(params[1]);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    float const* transform = reinterpret_cast<float const*>(mem_base + transform_ptr);

    returns[0] = WasmEdge_ValueGenI32(bridge->window_set_transform(static_cast<uint64_t>(window_internal), transform));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_window_set_alpha(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_window_set_alpha: memory not found");
        return WasmEdge_Result_Fail;
    }

    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_internal = WasmEdge_ValueGetI64(params[0]);
    int32_t const alpha_ptr = WasmEdge_ValueGetI32(params[1]);

    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    float const alpha = *reinterpret_cast<float const*>(mem_base + alpha_ptr);

    returns[0] = WasmEdge_ValueGenI32(bridge->window_set_alpha(static_cast<uint64_t>(window_internal), alpha));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_window_request_focus(
    void* data,
    WasmEdge_CallingFrameContext const*,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    int64_t const window_internal = WasmEdge_ValueGetI64(params[0]);
    returns[0] = WasmEdge_ValueGenI32(
        bridge->window_request_focus(static_cast<uint64_t>(window_internal)));
    return WasmEdge_Result_Success;
}

WasmEdge_Result host_miracle_get_plugin_userdata(
    void* data,
    WasmEdge_CallingFrameContext const* frame,
    WasmEdge_Value const* params,
    WasmEdge_Value* returns)
{
    auto const bridge = static_cast<PluginBridge*>(data);
    uint32_t const handle = static_cast<uint32_t>(WasmEdge_ValueGetI32(params[0]));
    int32_t const buf_ptr = WasmEdge_ValueGetI32(params[1]);
    int32_t const buf_len = WasmEdge_ValueGetI32(params[2]);

    auto const* userdata = bridge->get_plugin_userdata(handle);
    if (!userdata || userdata->empty())
    {
        returns[0] = WasmEdge_ValueGenI32(0);
        return WasmEdge_Result_Success;
    }
    if (userdata->size() + 1 > static_cast<size_t>(buf_len))
    {
        returns[0] = WasmEdge_ValueGenI32(-1);
        return WasmEdge_Result_Success;
    }
    auto* memory = get_memory_from_frame(frame);
    if (!memory)
    {
        mir::log_error("host_miracle_get_plugin_userdata: memory not found");
        return WasmEdge_Result_Fail;
    }
    uint8_t* mem_base = WasmEdge_MemoryInstanceGetPointer(memory, 0, 0);
    char* buf = reinterpret_cast<char*>(mem_base + buf_ptr);
    std::memcpy(buf, userdata->data(), userdata->size());
    buf[userdata->size()] = '\0';
    returns[0] = WasmEdge_ValueGenI32(static_cast<int32_t>(userdata->size()));
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

    add_host_function(module, "miracle_request_workspace",
        create_func_type({ i32, i32, i32, i32, i32, i32, i32, i32 }, { i32 }),
        host_miracle_request_workspace, bridge.get());

    add_host_function(module, "miracle_get_active_workspace",
        create_func_type({ i32, i32, i32 }, { i32 }),
        host_miracle_get_active_workspace, bridge.get());

    add_host_function(module, "miracle_num_managed_windows",
        create_func_type({ i32 }, { i32 }),
        host_miracle_num_managed_windows, bridge.get());

    add_host_function(module, "miracle_get_managed_window_at",
        create_func_type({ i32, i32, i32, i32, i32 }, { i32 }),
        host_miracle_get_managed_window_at, bridge.get());

    add_host_function(module, "miracle_window_set_state",
        create_func_type({ i64, i32 }, { i32 }),
        host_miracle_window_set_state, bridge.get());

    add_host_function(module, "miracle_window_set_workspace",
        create_func_type({ i64, i64 }, { i32 }),
        host_miracle_window_set_workspace, bridge.get());

    add_host_function(module, "miracle_window_set_rectangle",
        create_func_type({ i64, i32, i32, i32, i32, i32 }, { i32 }),
        host_miracle_window_set_rectangle, bridge.get());

    add_host_function(module, "miracle_window_set_transform",
        create_func_type({ i64, i32 }, { i32 }),
        host_miracle_window_set_transform, bridge.get());

    add_host_function(module, "miracle_window_set_alpha",
        create_func_type({ i64, i32 }, { i32 }),
        host_miracle_window_set_alpha, bridge.get());

    add_host_function(module, "miracle_window_request_focus",
        create_func_type({ i64 }, { i32 }),
        host_miracle_window_request_focus, bridge.get());

    add_host_function(module, "miracle_get_plugin_userdata",
        create_func_type({ i32, i32, i32 }, { i32 }),
        host_miracle_get_plugin_userdata, bridge.get());

    // Register the host module with the executor
    auto const r = WasmEdge_ExecutorRegisterImport(executor_context.get(), store_context.get(), module);
    if (!WasmEdge_ResultOK(r))
    {
        mir::log_error("Failed to register host module: %s", WasmEdge_ResultGetMessage(r));
        WasmEdge_ModuleInstanceDelete(module);
        return;
    }

    host_module.reset(module);
    mir::log_info("Host module 'env' registered with %d functions", 20);
}

std::vector<PluginManager::Self::ModuleInstance> PluginManager::Self::safe_copy()
{
    std::lock_guard lock(modules_access_mutex);
    return loaded_modules;
}

PluginLoadResult PluginManager::load_wasm_module(std::string const& path, std::string const& userdata_json)
{
    std::lock_guard lock(self->modules_access_mutex);
    auto const erased = std::erase_if(self->loaded_modules, [&path](auto const& module)
    {
        return module.name == path;
    });
    if (erased > 0)
        mir::log_info("Module with name %s was already loaded, unloading previous instance.", path.c_str());

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
    auto const module_name = WasmEdge_StringCreateByCString(path.c_str());
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

    // Assign handle before init so we can pass it to the plugin.
    auto const handle = self->next_plugin_handle++;

    // Store userdata so the plugin can retrieve it via the host function.
    if (!userdata_json.empty())
        self->bridge->set_plugin_userdata(handle, userdata_json);

    // Call the module's init function if it exists.
    auto const init_func_name = WasmEdge_StringCreateByCString("init");
    auto const init_func_context = WasmEdge_ModuleInstanceFindFunction(module_context, init_func_name);
    WasmEdge_StringDelete(init_func_name);
    if (init_func_context != nullptr)
    {
        WasmEdge_Value init_params[1];
        init_params[0] = WasmEdge_ValueGenI32(static_cast<int32_t>(handle));
        r = WasmEdge_ExecutorInvoke(self->executor_context.get(), init_func_context, init_params, 1, nullptr, 0);
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'init' function in module %s: %s", path.c_str(), WasmEdge_ResultGetMessage(r));
            return PluginLoadResult {
                .success = false,
                .error = "Failed to invoke 'init' function."
            };
        }
    }

    // Store the plugin for later. Function resolution happens at call time.
    self->loaded_modules.push_back(Self::ModuleInstance {
        Self::ModuleInstancePtr { module_context },
        handle,
        path });

    return PluginLoadResult {
        .success = true,
        .handle = handle
    };
}

bool PluginManager::unload_wasm_module(PluginHandle handle)
{
    std::lock_guard lock(self->modules_access_mutex);
    auto const erased = std::erase_if(self->loaded_modules, [handle](auto const& module)
    {
        return module.handle == handle;
    });
    return erased > 0;
}

void PluginManager::unload_all()
{
    std::lock_guard lock(self->modules_access_mutex);
    self->loaded_modules.clear();
}

std::optional<miracle_plugin_animation_frame_result_t> PluginManager::animate(
    miracle_plugin_animation_frame_data_t const& frame_data)
{
    auto const modules = self->safe_copy();
    for (auto const& target_module : modules)
    {
        // Get the memory context from the module instance
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(target_module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
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
            continue;
        }

        // Prepare the parameters
        // param[0]: pointer to frame_data
        // param[1]: pointer to result location
        WasmEdge_Value params[2];
        params[0] = WasmEdge_ValueGenI32(frame_data_ptr);
        params[1] = WasmEdge_ValueGenI32(result_ptr);

        // Call the function
        auto const func_name = WasmEdge_StringCreateByCString("animate");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(target_module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
        {
            mir::log_error("Function '%s' not found in module.", "window_open_animation");
            continue;
        }

        WasmEdge_Value returns[1];
        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            2,
            returns,
            1);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'animate' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        auto const animate_result = WasmEdge_ValueGetI32(returns[0]);
        if (animate_result == 0)
        {
            mir::log_info("Plugin did not handle 'animate' call (returned 0).");
            continue;
        }

        // Read the result from WASM memory
        miracle_plugin_animation_frame_result_t result;
        r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(&result), result_ptr, sizeof(result));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        mir::log_info("Successfully animated frame: completed=%d, has_area=%d, has_transform=%d, has_opacity=%d",
            result.completed, result.has_area, result.has_transform, result.has_opacity);
        return result;
    }

    return std::nullopt;
}

std::optional<PluginWindowPlacement> PluginManager::place_new_window(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification const& spec,
    uint64_t window_id)
{
    auto const bridge_handle = self->bridge->new_window_info(app_info, spec, window_id);
    auto const window_info_t = bridge_handle.get();
    for (auto const& module : self->safe_copy())
    {
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
            continue;
        }

        // Write window name to WASM memory
        auto const window_name = spec.name().value_or("");
        uint32_t const name_ptr = window_info_ptr + sizeof(miracle_window_info_t);
        uint32_t const name_len = static_cast<uint32_t>(window_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(window_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write window name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        // Prepare the parameters
        // param[0]: pointer to window_info
        // param[1]: pointer to result location
        // param[2]: pointer to window name
        // param[3]: length of window name
        WasmEdge_Value params[4];
        params[0] = WasmEdge_ValueGenI32(window_info_ptr);
        params[1] = WasmEdge_ValueGenI32(result_ptr);
        params[2] = WasmEdge_ValueGenI32(name_ptr);
        params[3] = WasmEdge_ValueGenI32(name_len);

        // Call the function
        auto const func_name = WasmEdge_StringCreateByCString("place_new_window");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
        {
            mir::log_error("Function 'place_new_window' not found in module.");
            continue;
        }

        WasmEdge_Value returns[1];
        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            4,
            returns,
            1);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'place_new_window' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        auto const placement_result = WasmEdge_ValueGetI32(returns[0]);
        if (placement_result == 0)
        {
            mir::log_info("Plugin did not handle 'animate' call (returned 0).");
            continue;
        }

        // Read the result from WASM memory
        miracle_placement_t result;
        r = WasmEdge_MemoryInstanceGetData(memory_context, reinterpret_cast<uint8_t*>(&result), result_ptr, sizeof(result));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to read result from WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        return from_c(result, module.handle);
    }

    return std::nullopt;
}

void PluginManager::window_deleted(miral::WindowInfo const& window_info)
{
    auto const bridge_handle = self->bridge->existing_window_info(window_info);
    auto const window_info_t = bridge_handle.get();
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr window_info_ptr = 0;

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
            continue;
        }

        auto const window_name = window_info.name();
        uint32_t const name_ptr = window_info_ptr + sizeof(miracle_window_info_t);
        uint32_t const name_len = static_cast<uint32_t>(window_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(window_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write window name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(window_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);

        auto const func_name = WasmEdge_StringCreateByCString("window_deleted");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'window_deleted' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::window_focused(miral::WindowInfo const& window_info)
{
    auto const bridge_handle = self->bridge->existing_window_info(window_info);
    auto const window_info_t = bridge_handle.get();
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr window_info_ptr = 0;

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
            continue;
        }

        auto const window_name = window_info.name();
        uint32_t const name_ptr = window_info_ptr + sizeof(miracle_window_info_t);
        uint32_t const name_len = static_cast<uint32_t>(window_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(window_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write window name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(window_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);

        auto const func_name = WasmEdge_StringCreateByCString("window_focused");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'window_focused' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::window_unfocused(miral::WindowInfo const& window_info)
{
    auto const bridge_handle = self->bridge->existing_window_info(window_info);
    auto const window_info_t = bridge_handle.get();
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr window_info_ptr = 0;

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
            continue;
        }

        auto const window_name = window_info.name();
        uint32_t const name_ptr = window_info_ptr + sizeof(miracle_window_info_t);
        uint32_t const name_len = static_cast<uint32_t>(window_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(window_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write window name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(window_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);

        auto const func_name = WasmEdge_StringCreateByCString("window_unfocused");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'window_unfocused' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::workspace_created(uint32_t id)
{
    auto const result = self->bridge->workspace_by_id(id);
    auto const& workspace_t = result.workspace;
    auto const& workspace_name = result.name.value_or("");
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr workspace_info_ptr = 0;

        uint8_t workspace_info_buffer[sizeof(miracle_workspace_t)];
        std::memcpy(workspace_info_buffer, &workspace_t, sizeof(workspace_t));
        auto r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            workspace_info_buffer,
            workspace_info_ptr,
            sizeof(workspace_info_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write workspace_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        uint32_t const name_ptr = workspace_info_ptr + sizeof(miracle_workspace_t);
        uint32_t const name_len = static_cast<uint32_t>(workspace_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(workspace_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write workspace name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(workspace_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);

        auto const func_name = WasmEdge_StringCreateByCString("workspace_created");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'workspace_created' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::workspace_removed(uint32_t id)
{
    auto const result = self->bridge->workspace_by_id(id);
    auto const& workspace_t = result.workspace;
    auto const& workspace_name = result.name.value_or("");
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr workspace_info_ptr = 0;

        uint8_t workspace_info_buffer[sizeof(miracle_workspace_t)];
        std::memcpy(workspace_info_buffer, &workspace_t, sizeof(workspace_t));
        auto r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            workspace_info_buffer,
            workspace_info_ptr,
            sizeof(workspace_info_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write workspace_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        uint32_t const name_ptr = workspace_info_ptr + sizeof(miracle_workspace_t);
        uint32_t const name_len = static_cast<uint32_t>(workspace_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(workspace_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write workspace name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(workspace_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);

        auto const func_name = WasmEdge_StringCreateByCString("workspace_removed");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'workspace_removed' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::workspace_focused(std::optional<uint32_t> previous_id, uint32_t current_id)
{
    auto const result = self->bridge->workspace_by_id(current_id);
    auto const& workspace_t = result.workspace;
    auto const& workspace_name = result.name.value_or("");
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr workspace_info_ptr = 0;

        uint8_t workspace_info_buffer[sizeof(miracle_workspace_t)];
        std::memcpy(workspace_info_buffer, &workspace_t, sizeof(workspace_t));
        auto r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            workspace_info_buffer,
            workspace_info_ptr,
            sizeof(workspace_info_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write workspace_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        uint32_t const name_ptr = workspace_info_ptr + sizeof(miracle_workspace_t);
        uint32_t const name_len = static_cast<uint32_t>(workspace_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(workspace_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write workspace name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        int32_t const has_previous = previous_id.has_value() ? 1 : 0;
        int64_t const previous_id_val = static_cast<int64_t>(previous_id.value_or(0));

        WasmEdge_Value params[5];
        params[0] = WasmEdge_ValueGenI32(workspace_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);
        params[3] = WasmEdge_ValueGenI32(has_previous);
        params[4] = WasmEdge_ValueGenI64(previous_id_val);

        auto const func_name = WasmEdge_StringCreateByCString("workspace_focused");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            5,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'workspace_focused' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::workspace_area_changed(uint32_t id)
{
    auto const result = self->bridge->workspace_by_id(id);
    auto const& workspace_t = result.workspace;
    auto const& workspace_name = result.name.value_or("");
    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr workspace_info_ptr = 0;

        uint8_t workspace_info_buffer[sizeof(miracle_workspace_t)];
        std::memcpy(workspace_info_buffer, &workspace_t, sizeof(workspace_t));
        auto r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            workspace_info_buffer,
            workspace_info_ptr,
            sizeof(workspace_info_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write workspace_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        uint32_t const name_ptr = workspace_info_ptr + sizeof(miracle_workspace_t);
        uint32_t const name_len = static_cast<uint32_t>(workspace_name.size());
        if (name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(workspace_name.data()),
                name_ptr,
                name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write workspace name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[3];
        params[0] = WasmEdge_ValueGenI32(workspace_info_ptr);
        params[1] = WasmEdge_ValueGenI32(name_ptr);
        params[2] = WasmEdge_ValueGenI32(name_len);

        auto const func_name = WasmEdge_StringCreateByCString("workspace_area_changed");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            3,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'workspace_area_changed' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

void PluginManager::window_workspace_changed(miral::WindowInfo const& window_info, uint32_t workspace_id)
{
    auto const bridge_window = self->bridge->existing_window_info(window_info);
    auto const window_info_t = bridge_window.get();
    auto const workspace_result = self->bridge->workspace_by_id(workspace_id);
    auto const& workspace_t = workspace_result.workspace;
    auto const& workspace_name = workspace_result.name.value_or("");
    auto const& window_name = window_info.name();

    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr window_info_ptr = 0;

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
            continue;
        }

        uint32_t const window_name_ptr = window_info_ptr + sizeof(miracle_window_info_t);
        uint32_t const window_name_len = static_cast<uint32_t>(window_name.size());
        if (window_name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(window_name.data()),
                window_name_ptr,
                window_name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write window name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        uint32_t const workspace_info_ptr = window_name_ptr + window_name_len;

        uint8_t workspace_info_buffer[sizeof(miracle_workspace_t)];
        std::memcpy(workspace_info_buffer, &workspace_t, sizeof(workspace_t));
        r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            workspace_info_buffer,
            workspace_info_ptr,
            sizeof(workspace_info_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write workspace_info to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        uint32_t const workspace_name_ptr = workspace_info_ptr + sizeof(miracle_workspace_t);
        uint32_t const workspace_name_len = static_cast<uint32_t>(workspace_name.size());
        if (workspace_name_len > 0)
        {
            r = WasmEdge_MemoryInstanceSetData(
                memory_context,
                reinterpret_cast<uint8_t const*>(workspace_name.data()),
                workspace_name_ptr,
                workspace_name_len);
            if (!WasmEdge_ResultOK(r))
            {
                mir::log_error("Failed to write workspace name to WASM memory: %s", WasmEdge_ResultGetMessage(r));
                continue;
            }
        }

        WasmEdge_Value params[6];
        params[0] = WasmEdge_ValueGenI32(window_info_ptr);
        params[1] = WasmEdge_ValueGenI32(window_name_ptr);
        params[2] = WasmEdge_ValueGenI32(window_name_len);
        params[3] = WasmEdge_ValueGenI32(workspace_info_ptr);
        params[4] = WasmEdge_ValueGenI32(workspace_name_ptr);
        params[5] = WasmEdge_ValueGenI32(workspace_name_len);

        auto const func_name = WasmEdge_StringCreateByCString("window_workspace_changed");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            6,
            nullptr,
            0);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'window_workspace_changed' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }
    }
}

bool PluginManager::handle_keyboard_event(MirKeyboardEvent const& event)
{
    miracle_keyboard_event_t const keyboard_event = {
        .action = static_cast<uint32_t>(miral::toolkit::mir_keyboard_event_action(&event)),
        .keysym = miral::toolkit::mir_keyboard_event_keysym(&event),
        .scan_code = miral::toolkit::mir_keyboard_event_scan_code(&event),
        .modifiers = static_cast<uint32_t>(miral::toolkit::mir_keyboard_event_modifiers(&event)),
    };

    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr event_ptr = 0;

        uint8_t event_buffer[sizeof(miracle_keyboard_event_t)];
        std::memcpy(event_buffer, &keyboard_event, sizeof(keyboard_event));
        auto r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            event_buffer,
            event_ptr,
            sizeof(event_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write keyboard_event to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        WasmEdge_Value params[1];
        params[0] = WasmEdge_ValueGenI32(event_ptr);

        auto const func_name = WasmEdge_StringCreateByCString("handle_keyboard_input");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        WasmEdge_Value returns[1];
        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            1,
            returns,
            1);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'handle_keyboard_input' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        if (WasmEdge_ValueGetI32(returns[0]) != 0)
            return true;
    }

    return false;
}

bool PluginManager::handle_pointer_event(MirPointerEvent const& event)
{
    miracle_pointer_event_t const pointer_event = {
        .x = miral::toolkit::mir_pointer_event_axis_value(&event, MirPointerAxis::mir_pointer_axis_x),
        .y = miral::toolkit::mir_pointer_event_axis_value(&event, MirPointerAxis::mir_pointer_axis_y),
        .action = static_cast<uint32_t>(miral::toolkit::mir_pointer_event_action(&event)),
        .modifiers = static_cast<uint32_t>(miral::toolkit::mir_pointer_event_modifiers(&event)),
        .buttons = static_cast<uint32_t>(mir_pointer_event_buttons(&event)),
    };

    for (auto const& module : self->safe_copy())
    {
        auto const memory_name = WasmEdge_StringCreateByCString("memory");
        auto const memory_context = WasmEdge_ModuleInstanceFindMemory(module.module_context.get(), memory_name);
        WasmEdge_StringDelete(memory_name);

        if (memory_context == nullptr)
        {
            mir::log_error("Memory not found in module.");
            continue;
        }

        uint32_t constexpr event_ptr = 0;

        uint8_t event_buffer[sizeof(miracle_pointer_event_t)];
        std::memcpy(event_buffer, &pointer_event, sizeof(pointer_event));
        auto r = WasmEdge_MemoryInstanceSetData(
            memory_context,
            event_buffer,
            event_ptr,
            sizeof(event_buffer));
        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to write pointer_event to WASM memory: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        WasmEdge_Value params[1];
        params[0] = WasmEdge_ValueGenI32(event_ptr);

        auto const func_name = WasmEdge_StringCreateByCString("handle_pointer_event");
        auto const func_context = WasmEdge_ModuleInstanceFindFunction(module.module_context.get(), func_name);
        WasmEdge_StringDelete(func_name);

        if (func_context == nullptr)
            continue;

        WasmEdge_Value returns[1];
        r = WasmEdge_ExecutorInvoke(
            self->executor_context.get(),
            func_context,
            params,
            1,
            returns,
            1);

        if (!WasmEdge_ResultOK(r))
        {
            mir::log_error("Failed to invoke 'handle_pointer_event' function: %s", WasmEdge_ResultGetMessage(r));
            continue;
        }

        if (WasmEdge_ValueGetI32(returns[0]) != 0)
            return true;
    }

    return false;
}

PluginWindowPlacement PluginManager::from_c(miracle_placement_t placement, PluginHandle plugin_handle)
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
        result.freestyle.handle = plugin_handle;
        std::memcpy(&result.freestyle.transform, placement.freestyle_placement.transform, sizeof(float) * 16);
        result.freestyle.alpha = placement.freestyle_placement.alpha;
        result.freestyle.resizable = placement.freestyle_placement.resizable;
        result.freestyle.movable = placement.freestyle_placement.movable;
        break;
    }
    default:
        break;
    }
    return result;
}

#endif
