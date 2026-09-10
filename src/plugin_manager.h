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
#include "../miracle-plugin-rs/plugin.h"
#include "layout_scheme.h"
#include "plugin_handle.h"
#include <glm/glm.hpp>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miracle/cpp/config-cpp.h>
#include <miral/toolkit_event.h>
#include <optional>
#include <string>

namespace miral
{
struct ApplicationInfo;
class WindowSpecification;
struct WindowInfo;
}

namespace miracle
{
class Container;
class AbstractWorkspace;

struct PluginWindowPlacement
{
    struct TiledPlacement
    {
        Container* container;
        uint32_t index;
        LayoutScheme scheme;
    };

    struct FreestylePlacement
    {
        /// The rectangle of the window.
        mir::geometry::Rectangle rectangle;

        /// The layer of the window.
        MirDepthLayer layer;

        /// The workspace of the window.
        AbstractWorkspace* workspace;

        /// The plugin handle that is managing the window.
        PluginHandle handle;

        /// The initial 4x4 transform applied to the window.
        glm::mat4 transform = glm::mat4(1.f);

        /// The initial alpha (opacity) of the window.
        float alpha = 1.f;

        /// Whether the window can be resized.
        bool resizable = true;

        /// Whether the window can be moved.
        bool movable = true;
    };

    miracle_window_management_strategy_t strategy = miracle_window_management_strategy_system;
    TiledPlacement tiled;
    FreestylePlacement freestyle;
};

struct AnimationData;
class PluginBridge;

struct PluginLoadResult
{
    bool success = false;
    PluginHandle handle = 0;
    std::string error;
};

/// Abstract interface to the plugin system.
///
/// The concrete, WasmEdge-backed implementation lives in a separate shared
/// library (libmiracle-wm-plugins.so) that is loaded with `dlopen`/`RTLD_LOCAL`
/// at runtime by #load_plugin_manager(). Keeping the implementation out of the
/// main binary keeps WasmEdge's bundled LLVM out of the global symbol scope,
/// where it would otherwise interpose on Mesa's software renderer (llvmpipe)
/// and break rendering (see issue #865). All callers interact with the plugin
/// system through this interface so the main binary never links WasmEdge.
class PluginManager
{
public:
    virtual ~PluginManager() = default;

    /// Initialize the plugin bridge.
    ///
    /// This must be called after construction.
    virtual void initialize(std::unique_ptr<PluginBridge> bridge) = 0;

    /// Load a WebAssembly module from the provided \p path.
    ///
    /// Callers may use the #PluginLoadResult::handle to unload the module later.
    ///
    /// \param path The filesystem path to the WebAssembly module.
    /// \param userdata_json Optional JSON-encoded user data to associate with the plugin.
    /// \returns a load result.
    virtual PluginLoadResult load_wasm_module(std::string const& path, std::string const& userdata_json = "") = 0;

    /// Attempt to unload the WebAssembly module associated with \p handle.
    ///
    /// \returns `true` if the module was unloaded, `false` otherwise.
    virtual bool unload_wasm_module(PluginHandle handle) = 0;

    /// Unload all loaded WebAssembly modules.
    virtual void unload_all() = 0;

    /// Animate a frame via the plugin system.
    ///
    /// \param data The animation data to animate.
    /// \param runtime_seconds The current runtime of the animation in seconds.
    /// \returns The result of the animation frame, or none if none is set.
    virtual std::optional<miracle_plugin_animation_frame_result_t> animate(
        AnimationData const& data, float runtime_seconds)
        = 0;

    /// Tick a custom animation for the given plugin.
    ///
    /// \param plugin_handle The handle of the plugin that owns this animation.
    /// \param animation_id The host-generated animation ID.
    /// \param dt The time delta in seconds since the last tick.
    /// \param elapsed_seconds The cumulative elapsed time in seconds since the animation started.
    virtual void custom_animate(PluginHandle plugin_handle, uint32_t animation_id, float dt, float elapsed_seconds) = 0;

    /// Try to place a new window using a plugin.
    ///
    /// \param app_info The application info.
    /// \param spec The window specification.
    /// \returns the placement
    virtual std::optional<PluginWindowPlacement> place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& spec,
        uint64_t window_id)
        = 0;

    /// Notify all plugins that a window has been deleted.
    virtual void window_deleted(miral::WindowInfo const& window_info) = 0;

    /// Notify all plugins that a window has been focused.
    virtual void window_focused(miral::WindowInfo const& window_info) = 0;

    /// Notify all plugins that a window has been unfocused.
    virtual void window_unfocused(miral::WindowInfo const& window_info) = 0;

    /// Notify all plugins that a workspace has been created.
    virtual void workspace_created(uint32_t id) = 0;

    /// Notify all plugins that a workspace has been removed.
    virtual void workspace_removed(uint32_t id) = 0;

    /// Notify all plugins that a workspace has gained focus.
    virtual void workspace_focused(std::optional<uint32_t> previous_id, uint32_t current_id) = 0;

    /// Notify all plugins that a workspace's area has changed.
    virtual void workspace_area_changed(uint32_t id) = 0;

    /// Notify all plugins that a window's workspace has changed.
    virtual void window_workspace_changed(miral::WindowInfo const& window_info, uint32_t workspace_id) = 0;

    /// Check if the plugin handles a keyboard event.
    ///
    /// \returns `true` if the keyboard event was consumed, otherwise `false`
    virtual bool handle_keyboard_event(MirKeyboardEvent const& event) = 0;

    /// Check if the plugin handles a pointer event.
    ///
    /// \returns `true` if the pointer event was consumed, otherwise `false`
    virtual bool handle_pointer_event(MirPointerEvent const& event) = 0;

    /// Call the configure() export on every loaded plugin and merge all of their
    /// results into a single PluginConfigData.
    virtual PluginConfigData configure() = 0;

    /// Route an IPC command to the plugin that owns \p ns.
    ///
    /// The plugin handles the arbitrary JSON \p payload_json and may return an arbitrary
    /// JSON response.
    ///
    /// \returns the plugin's JSON response, or std::nullopt if no plugin owns \p ns (or
    ///          the owning plugin does not implement the handler).
    virtual std::optional<std::string> handle_plugin_command(
        std::string const& ns, std::string const& payload_json)
        = 0;
};

/// Returns a no-op PluginManager. Used when the plugin feature is disabled at
/// compile time, or as a fallback when the plugin module cannot be loaded.
std::shared_ptr<PluginManager> make_null_plugin_manager();

/// Loads the real, WasmEdge-backed PluginManager from the isolated plugin module
/// (libmiracle-wm-plugins.so) when the plugin feature is enabled; otherwise, or
/// on any failure, returns make_null_plugin_manager().
std::shared_ptr<PluginManager> load_plugin_manager();
}

#endif // MIRACLEWM_PLUGIN_MANAGER_H
