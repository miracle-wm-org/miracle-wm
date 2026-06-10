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

#ifndef MIRACLEWM_PLUGIN_MANAGER_IMPL_H
#define MIRACLEWM_PLUGIN_MANAGER_IMPL_H

#include "plugin_manager.h"

#include <memory>
#include <mir/geometry/point.h>
#include <mutex>
#include <vector>
#include <wasmedge/wasmedge.h>

namespace miracle
{
class PluginBridge;

/// Concrete, WasmEdge-backed implementation of #PluginManager.
///
/// This class (and everything it pulls in, notably WasmEdge and its bundled
/// LLVM) lives exclusively in the libmiracle-wm-plugins.so shared library so it
/// can be loaded with RTLD_LOCAL and never pollutes the main binary's global
/// symbol scope. See plugin_manager.h for the rationale (issue #865).
class PluginManagerImpl final : public PluginManager
{
public:
    ~PluginManagerImpl() override;

    void initialize(std::unique_ptr<PluginBridge> bridge) override;
    PluginLoadResult load_wasm_module(std::string const& path, std::string const& userdata_json) override;
    bool unload_wasm_module(PluginHandle handle) override;
    void unload_all() override;
    std::optional<miracle_plugin_animation_frame_result_t> animate(
        AnimationData const& data, float runtime_seconds) override;
    void custom_animate(PluginHandle plugin_handle, uint32_t animation_id, float dt, float elapsed_seconds) override;
    std::optional<PluginWindowPlacement> place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& spec,
        uint64_t window_id) override;
    void window_deleted(miral::WindowInfo const& window_info) override;
    void window_focused(miral::WindowInfo const& window_info) override;
    void window_unfocused(miral::WindowInfo const& window_info) override;
    void workspace_created(uint32_t id) override;
    void workspace_removed(uint32_t id) override;
    void workspace_focused(std::optional<uint32_t> previous_id, uint32_t current_id) override;
    void workspace_area_changed(uint32_t id) override;
    void window_workspace_changed(miral::WindowInfo const& window_info, uint32_t workspace_id) override;
    bool handle_keyboard_event(MirKeyboardEvent const& event) override;
    bool handle_pointer_event(MirPointerEvent const& event) override;
    PluginConfigData configure() override;

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

    /// Broadcast a window-info-only callback (export \p fn_name) to every loaded plugin.
    /// Writes the window_info struct followed by its name into each module's memory.
    void dispatch_window_event(char const* fn_name, miral::WindowInfo const& window_info);

    /// Broadcast a workspace-info-only callback (export \p fn_name) to every loaded plugin.
    /// Writes the workspace struct followed by its name into each module's memory.
    void dispatch_workspace_event(char const* fn_name, uint32_t id);

    std::mutex mutex_;
    std::unique_ptr<Self> self;
};
}

#endif // MIRACLEWM_PLUGIN_MANAGER_IMPL_H
