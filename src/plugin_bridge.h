/**
Copyright (C) 2024  Matthew Kosarek

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

#include "miracle/plugin.h"
#include <functional>
#include <miral/window_info.h>
#include <miral/window_specification.h>
#include <variant>

namespace miracle
{
class Container;
class OutputManager;
class WindowManagerToolsWindowController;

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

    PluginBridge(
        std::shared_ptr<OutputManager> const& output_manager,
        std::shared_ptr<WindowManagerToolsWindowController> const& window_controller);
    miracle_application_info_t application(uint64_t window_id);
    miracle_workspace_t workspace(miracle_window_info_t const& window_info);
    miracle_output_t output(miracle_workspace_t const& workspace);
    uint32_t num_outputs();
    OutputResult output_at(uint32_t index);
    uint32_t num_workspaces_on_output(miracle_output_t const& output);
    miracle_workspace_t workspace_on_output_at(miracle_output_t const& output, uint32_t index);
    miracle_container_t tree_at_index(miracle_workspace_t const& workspace, uint32_t index);
    miracle_container_t child_at(miracle_container_t const& parent, uint32_t index);
    miracle_window_info_t get_window(miracle_container_t const& container);
    PluginBridgeObjectHandle<miracle_window_info_t> new_window_info(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& spec);

private:
    /// This is the information expected to be on a #miracle_window_info_t.
    struct PluginWindowInfo
    {
        miral::ApplicationInfo const& app_info;
        std::variant<miral::WindowSpecification, miral::Window> window_info;
    };

    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WindowManagerToolsWindowController> window_controller;
    std::vector<std::shared_ptr<PluginWindowInfo>> plugin_window_infos;
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
