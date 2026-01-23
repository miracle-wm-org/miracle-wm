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

miracle_application_info_t miracle_window_info_get_application(miracle_context_t* context, miracle_window_info_t* window_info)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->application(*window_info);
}

miracle_workspace_t miracle_window_info_get_workspace(miracle_context_t* context, miracle_window_info_t* window_info)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->workspace(*window_info);
}

miracle_output_t miracle_workspace_get_output(miracle_context_t* context, miracle_workspace_t* workspace)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->output(*workspace);
}

uint32_t miracle_get_num_outputs(miracle_context_t* context)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->num_outputs();
}

miracle_output_t miracle_get_output_at(miracle_context_t* context, uint32_t index)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->output_at(index);
}

miracle_container_t miracle_workspace_get_tree(miracle_context_t* context, miracle_workspace_t* workspace, uint32_t index)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->tree_at_index(*workspace, index);
}

miracle_container_t miracle_container_get_child_at(miracle_context_t* context, miracle_container_t* container, uint32_t index)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->child_at(*container, index);
}

miracle_window_info_t miracle_container_get_window(miracle_context_t* context, miracle_container_t* container)
{
    auto const bridge = static_cast<miracle::PluginBridge*>(context->internal);
    return bridge->get_window(*container);
}