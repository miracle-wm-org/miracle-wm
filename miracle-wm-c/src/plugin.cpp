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

#include "miracle/plugin.h"
#include "miracle/cpp/plugin_bridge.h"

// TODO: Need to move "Container" class to the miracle-wm-config library
// TODO: Need to rename miracle-wm-config to just libmiracle or similar
// TODO: Plugin objects should drag around a "plugin interface" pointer that
//       has access to things like the window manager tools.
// TODO: Maybe the plugins work belongs to a separate library that does
//       not include the config stuff.

miracle_application_info_t miracle_plugin_get_application(miracle_context_t* context, miracle_window_info_t* window_info)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->application(*window_info);
}

miracle_workspace_t miracle_plugin_get_workspace_from_window(miracle_context_t* context, miracle_window_info_t* window_info)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->workspace(*window_info);
}

miracle_output_t miracle_plugin_get_output_from_workspace(miracle_context_t* context, miracle_workspace_t* workspace)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->output(*workspace);
}

uint32_t miracle_plugin_num_outputs(miracle_context_t* context)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->num_outputs();
}

miracle_output_t miracle_plugin_get_output(miracle_context_t* context, uint32_t index)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->output_at(index);
}
