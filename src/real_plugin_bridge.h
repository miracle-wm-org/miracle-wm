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

#ifndef MIRACLE_REAL_PLUGIN_BRIDGE_H
#define MIRACLE_REAL_PLUGIN_BRIDGE_H

#include "miracle/cpp/plugin_bridge.h"
#include <miral/window_info.h>
#include <miral/window_specification.h>
#include <variant>

namespace miracle
{
class Container;
class OutputManager;
class WindowManagerToolsWindowController;

/// This is the information expected to be on a #miracle_window_info_t.
///
/// This is implied for now, but that is okay in my opinion since the
/// information is local.
struct PluginWindowInfo
{
    miral::ApplicationInfo const& app_info;
    std::variant<miral::WindowSpecification, miral::WindowInfo> window_info;
};

class RealPluginBridge : public PluginBridge
{
public:
    RealPluginBridge(
        std::shared_ptr<OutputManager> const& output_manager,
        std::shared_ptr<WindowManagerToolsWindowController> const& window_controller);
    miracle_application_info_t application(miracle_window_info_t const& window_info) override;
    miracle_workspace_t workspace(miracle_window_info_t const& window_info) override;
    miracle_output_t output(miracle_workspace_t const& workspace) override;
    uint32_t num_outputs() override;
    miracle_output_t output_at(uint32_t index) override;
    uint32_t num_workspaces_on_output(miracle_output_t const& output) override;
    miracle_workspace_t workspace_on_output_at(miracle_output_t const& output, uint32_t index) override;

private:
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WindowManagerToolsWindowController> window_controller;
};

miracle_window_info_t new_window_info(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& spec);
void free_window_info(miracle_window_info_t const& window_info);
}

#endif
