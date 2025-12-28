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

#ifndef MIRACLE_WM_SHELL_APPLICATION_SPAWNER_H
#define MIRACLE_WM_SHELL_APPLICATION_SPAWNER_H

#include <miral/application.h>

namespace miracle
{
class Container;

enum class ShellApplicationRole
{
    parent_container_background
};

/// Used to notify delegates about shell component events.
class ShellApplicationDelegate
{
public:
    virtual ~ShellApplicationDelegate() = default;
    virtual void handle_ready(std::shared_ptr<Container> const&) = 0;
};

class ShellApplication
{
public:
    virtual ~ShellApplication() = default;
    virtual void stop() = 0;
    virtual miral::Application application() = 0;
};

/// Spawns shell applications.
class ShellApplicationSpawner
{
public:
    virtual ~ShellApplicationSpawner() = default;

    /// Spawns a shell application with the given \p type.
    ///
    /// \param role the application that should be spawned
    /// \returns the spawned shell application
    virtual std::unique_ptr<ShellApplication> spawn(ShellApplicationRole role) = 0;
};
}

#endif