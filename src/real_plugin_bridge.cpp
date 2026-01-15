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

#include "real_plugin_bridge.h"

using namespace miracle;

namespace
{
}

miracle_application_info_t RealPluginBridge::application(miracle_window_info_t const& window_info)
{
    auto const plugin_window_info = reinterpret_cast<PluginWindowInfo*>(window_info.internal);
    if (std::holds_alternative<miral::WindowInfo>(plugin_window_info->window_info))
    {
        auto const& miral_window_info = std::get<miral::WindowInfo>(plugin_window_info->window_info);
    }
    else
    {
    }
}

miracle_workspace_t RealPluginBridge::workspace(miracle_window_info_t const& window_info)
{
}

miracle_output_t RealPluginBridge::output(miracle_workspace_t const& workspace)
{
}

uint32_t RealPluginBridge::num_outputs()
{
}

miracle_output_t RealPluginBridge::output_by_index(uint32_t index)
{
}
