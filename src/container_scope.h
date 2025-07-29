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

#ifndef MIRACLE_CONTAINER_SCOPE_H
#define MIRACLE_CONTAINER_SCOPE_H

#include <memory>
#include <string>

namespace miracle
{
class Container;

// https://i3wm.org/docs/userguide.html#command_criteria
enum class ContainerScopeType
{
    none,
    all,
    machine,
    title,
    urgent,
    workspace,
    con_mark,
    con_id,
    floating,
    floating_from,
    tiling,
    tiling_from,
    app_id,
    pid,

    /// TODO: X11-only
    class_,
    /// TODO: X11-only
    instance,
    /// TODO: X11-only
    window_role,
    /// TODO: X11-only
    window_type,
    // TODO: X11-only
    id,
};

/// Provided to methods in the [CommandController] to
/// describe which container(s) the command should act
/// on.
class ContainerScope
{
public:
    explicit ContainerScope(ContainerScopeType type) :
        ContainerScope(type, "")
    {
    }

    ContainerScope(ContainerScopeType type, std::string const& value) :
        ContainerScope(type, value, {})
    {
    }

    ContainerScope(
        ContainerScopeType type,
        std::string const& value,
        std::weak_ptr<Container> const& container) :
        type(type),
        value(value),
        container(container)
    {
    }

    /// Create a scope that targets a specific container
    static ContainerScope from_container(std::shared_ptr<Container> const& container)
    {
        return ContainerScope(ContainerScopeType::none, "", container);
    }

    /// Create a scope that matches all containers
    static ContainerScope all()
    {
        return ContainerScope(ContainerScopeType::all, "");
    }

    ContainerScopeType type = ContainerScopeType::none;
    std::string value;
    std::weak_ptr<Container> container;
};

}

#endif
