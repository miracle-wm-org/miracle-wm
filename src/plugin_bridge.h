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

#ifndef MIRACLE_PLUGIN_BRIDGE_H
#define MIRACLE_PLUGIN_BRIDGE_H

#include "../miracle-plugin-rs/plugin.h"
#include "compositor_state.h"
#include "sampler_registry.h"
#include "window_id_map.h"
#include <functional>
#include <miral/application_info.h>
#include <miral/window.h>
#include <miral/window_info.h>
#include <miral/window_specification.h>
#include <unordered_map>
#include <variant>

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{
class AbstractOutput;
class AbstractWorkspace;
class Animator;
class CompositorState;
class Container;
class OutputManager;
class PluginManager;
class WindowContainer;
class WindowController;
class WorkspaceManager;

using ApplicationIdMap = std::unordered_map<uint64_t, miral::Application>;

/// Convert an AbstractWorkspace to its C plugin representation.
miracle_workspace_t from_workspace(std::shared_ptr<AbstractWorkspace> const& workspace);

/// Convert a miral::WindowInfo to its C plugin representation.
///
/// Pass \p internal as 0 for read-only animation context where no stable ID is available.
/// Pass \p container if you want transform and alpha to be copied from the container.
miracle_window_info_t from_window(
    miral::WindowInfo const& window_info,
    uint64_t internal,
    WindowContainer* container = nullptr);

template <typename T>
class PluginBridgeObjectHandle
{
public:
    explicit PluginBridgeObjectHandle(T&& o, std::function<void()>&& deleter) :
        o(std::move(o)),
        deleter(std::move(deleter))
    {
    }
    ~PluginBridgeObjectHandle()
    {
        deleter();
    }

    T get() const { return o; }

private:
    T o;
    std::function<void()> deleter;
};

/// The bridge between the #PluginManager and the rest of the system.
///
/// The plugin manager will forward requests made on its host functions
/// to the bridge. The bridge will then return the right data to the plugin
/// manager.
class PluginBridge
{
public:
    struct OutputResult
    {
        miracle_output_t output;
        std::string name;
    };

    struct WorkspaceResult
    {
        miracle_workspace_t workspace;
        std::optional<std::string> name;
    };

    struct WindowResult
    {
        miracle_window_info_t window_info;
        std::string name;
    };

    PluginBridge(
        std::shared_ptr<OutputManager> const& output_manager,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<WorkspaceManager> const& workspace_manager,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<WindowIdMap> const& window_id_map,
        std::shared_ptr<ApplicationIdMap> const& application_id_map,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
        std::shared_ptr<SamplerRegistry> const& sampler_registry);

    miracle_application_info_t application_from_window(uint64_t window_id);
    WorkspaceResult workspace_from_window(uint64_t window_id);
    uint32_t num_outputs();
    OutputResult output_at(uint32_t index);
    OutputResult output_from_workspace(uint64_t workspace_id);
    uint32_t num_workspaces_on_output(uint64_t workspace_id);
    WorkspaceResult workspace_on_output_at_index(uint64_t output_id, uint32_t index);
    miracle_container_t root_tree(uint64_t workspace_id);
    miracle_container_t child_at(uint64_t parent_id, uint32_t index);
    miracle_container_t container_from_window(uint64_t window_id);
    miracle_container_t parent_from_container(uint64_t container_id);
    WindowResult get_window(uint64_t container_address);
    WorkspaceResult request_workspace(std::optional<int> num, std::optional<std::string> name, bool focus);
    WorkspaceResult active_workspace();
    uint32_t num_managed_windows(uint32_t plugin_handle);
    WindowResult get_managed_window_at(uint32_t plugin_handle, uint32_t index);
    int32_t queue_custom_animation(uint32_t plugin_handle,
        uint32_t* out_animation_id,
        PluginManager* manager,
        float duration_seconds);

    uint8_t register_window_shader(std::vector<std::string> passes);

    int32_t window_set_state(uint64_t window_internal, int32_t state);
    int32_t window_set_workspace(uint64_t window_internal, uint64_t workspace_internal);
    int32_t window_set_rectangle(uint64_t window_internal, int32_t x, int32_t y, int32_t width, int32_t height, bool animate);
    int32_t window_set_transform(uint64_t window_internal, float const* transform);
    int32_t window_set_alpha(uint64_t window_internal, float alpha);
    int32_t window_request_focus(uint64_t window_internal);
    int32_t window_set_shader_id(uint64_t window_internal, int32_t shader_id_param);

    void set_plugin_userdata(uint32_t handle, std::string const& userdata_json);
    std::string const* get_plugin_userdata(uint32_t handle) const;

    /// Look up a workspace by its ID and return its plugin representation.
    WorkspaceResult workspace_by_id(uint32_t id);

    PluginBridgeObjectHandle<miracle_window_info_t> new_window_info(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& spec, uint64_t window_id);
    PluginBridgeObjectHandle<miracle_window_info_t> existing_window_info(miral::WindowInfo const& window_info);

    /// Retrieve the container from its id.
    Container* resolve_container(uint64_t container_internal);

    /// Retrieve the workspace from its id.
    AbstractWorkspace* resolve_workspace(uint64_t workspace_internal);

private:
    std::shared_ptr<AbstractOutput> resolve_output(uint64_t output_id);
    std::shared_ptr<AbstractWorkspace> resolve_workspace_shared(uint64_t workspace_id);
    /// Walk the container tree and return the container with the given \p id, or nullptr.
    std::shared_ptr<Container> find_container_by_id(uint64_t id);
    uint64_t find_window_id(miral::Window const& window) const;
    uint64_t find_application_id(miral::Application const& app) const;
    /// This is the information expected to be on a #miracle_window_info_t.
    struct PluginWindowInfo
    {
        miral::ApplicationInfo const& app_info;
        std::variant<miral::WindowSpecification, miral::Window> window_info;
    };

    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<WindowIdMap> window_id_map;
    std::shared_ptr<ApplicationIdMap> application_id_map;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::shared_ptr<SamplerRegistry> sampler_registry_;
    uint32_t next_animation_id = 1;
    std::vector<std::shared_ptr<PluginWindowInfo>> plugin_window_infos;
    std::unordered_map<uint32_t, std::string> plugin_userdata_map;
};

inline miracle_point_t from_point(mir::geometry::Point const& point)
{
    return { point.x.as_int(), point.y.as_int() };
}

inline mir::geometry::Point from_point(miracle_point_t const& point)
{
    return { point.x, point.y };
}

inline miracle_size_t from_size(mir::geometry::Size const& size)
{
    return { size.width.as_int(), size.height.as_int() };
}

inline mir::geometry::Size from_size(miracle_size_t const& size)
{
    return { size.w, size.h };
}

}

#endif
