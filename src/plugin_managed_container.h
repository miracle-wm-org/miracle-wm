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

#ifndef MIRACLE_PLUGIN_MANAGED_CONTAINER
#define MIRACLE_PLUGIN_MANAGED_CONTAINER

#include "container.h"
#include "plugin_manager.h"

namespace miracle
{
/// A container that is managed by a plugin.
///
/// The idea of this class is to allow plugins to create and manage
/// containers that can be used by Miracle in the same way as normal
/// containers. This would allow for more advanced plugin functionality,
/// such as custom tiling algorithms or container behaviors.
///
/// Unlike #ShellComponentContainer, a plugin managed container can be
/// associated with a specific workspace. However, it is *not* associated
/// with a tiling grid, like other containers.
class PluginManagedContainer : public Container
{
public:
    PluginManagedContainer(
        PluginHandle plugin_handle,
        miral::Window const& window);

private:
    PluginHandle plugin_handle;
};
}

#endif