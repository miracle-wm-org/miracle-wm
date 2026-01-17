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

#include "container.h"
#include "real_plugin_bridge.h"
#include "output_interface.h"
#include "output_manager.h"
#include "window_manager_tools_window_controller.h"
#include "workspace_interface.h"
#include <miral/application_info.h>

using namespace miracle;

namespace
{
miracle_application_info_t from_app_info(miral::ApplicationInfo const& info)
{
    return {
        .application_name = info.name().c_str()
    };
}

miracle_workspace_t from_workspace(std::shared_ptr<WorkspaceInterface> const& workspace)
{
    if (workspace == nullptr)
        return { .is_set = false };

    return {
        .is_set = true,
        .has_number = workspace->num().has_value(),
        .number = static_cast<uint32_t>(workspace->num().value_or(0)),
        .has_name = workspace->name().has_value(),
        .name = workspace->name().value_or("").c_str(),
        .internal = static_cast<void*>(workspace.get())
    };
}

miracle_output_t from_output(std::shared_ptr<OutputInterface> const& output)
{
    if (output == nullptr)
        return { .is_set = false };

    auto const area = output->get_area();
    return {
        .is_set = true,
        .position = miracle_point_t { area.top_left.x.as_int(), area.top_left.y.as_int() },
        .size = miracle_size_t { area.size.width.as_int(), area.size.height.as_int() },
        .name = output->name().c_str(),
        .is_primary = output->is_primary(),
        .internal = static_cast<void*>(output.get())
    };
}
}

miracle_application_info_t RealPluginBridge::application(miracle_window_info_t const& window_info)
{
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(window_info.internal);
    return from_app_info(plugin_window_info->app_info);
}

miracle_workspace_t RealPluginBridge::workspace(miracle_window_info_t const& window_info)
{
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(window_info.internal);
    if (std::holds_alternative<miral::WindowInfo>(plugin_window_info->window_info))
    {
        auto const& miral_window_info = std::get<miral::WindowInfo>(plugin_window_info->window_info);
        if (auto const container = window_controller->get_container(miral_window_info.window()))
            return from_workspace(container->get_workspace());
    }

    return from_workspace(nullptr);
}

miracle_output_t RealPluginBridge::output(miracle_workspace_t const& workspace)
{
    auto const miracle_workspace = static_cast<WorkspaceInterface*>(workspace.internal);
    if (!miracle_workspace)
    {
        return {
            .is_set = false,
        };
    }

    return from_output(miracle_workspace->get_output());
}

uint32_t RealPluginBridge::num_outputs()
{
    return output_manager->outputs().size();
}

miracle_output_t RealPluginBridge::output_at(uint32_t index)
{
    return from_output(output_manager->outputs()[index]);
}

uint32_t RealPluginBridge::num_workspaces_on_output(miracle_output_t const& output)
{
    auto const miracle_output = static_cast<OutputInterface*>(output.internal);
    if (!miracle_output)
        return 0;

    return miracle_output->get_workspaces().size();
}

miracle_workspace_t RealPluginBridge::workspace_on_output_at(miracle_output_t const& output, uint32_t index)
{
    auto const miracle_output = static_cast<OutputInterface*>(output.internal);
    if (!miracle_output)
        return from_workspace(nullptr);

    return from_workspace(miracle_output->get_workspaces()[index]);
}
