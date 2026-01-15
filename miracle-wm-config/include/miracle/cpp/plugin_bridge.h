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

#include "../plugin.h"
#include <memory>

namespace miracle
{
/// This class is provided to plugin objects as the internal pointer.
/// Implementers of this class provide methods so that plugins can easily
/// resolve information from existing objects.
///
/// Miracle itself will provide an implementation that reaches out to its
/// workspace, output, and application management system.
class PluginBridge
{
public:
    virtual ~PluginBridge() = default;
    virtual miracle_application_info_t application(miracle_window_info_t const& window_info) = 0;
    virtual miracle_workspace_t workspace(miracle_window_info_t const& window_info) = 0;
    virtual miracle_output_t output(miracle_workspace_t const& workspace) = 0;
    virtual uint32_t num_outputs() = 0;
    virtual miracle_output_t output_by_index(uint32_t index) = 0;
};
}

#endif
