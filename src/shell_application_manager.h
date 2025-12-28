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

#ifndef MIRACLE_WM_SHELL_APPLICATION_MANAGER_H
#define MIRACLE_WM_SHELL_APPLICATION_MANAGER_H

#include "shell_application_spawner.h"
#include <vector>

namespace miracle
{
typedef uint32_t ShellApplicationId;

/// Manages applications that have special shell roles.
///
///
class ShellApplicationManager
{
public:
    explicit ShellApplicationManager(std::unique_ptr<ShellApplicationSpawner> spawner);

    /// Spawn application with a specific \p role.
    ///
    /// The behavior of the application will be provided by the \p delegate.
    ///
    /// \param role the shell application role
    /// \param delegate controls the behavior of the application
    /// \returns a unique id
    ShellApplicationId spawn(ShellApplicationRole role, std::shared_ptr<ShellApplicationDelegate> const& delegate);

    /// Stops the application with the provided id.
    void stop(ShellApplicationId app_id);

    /// Check if an application is registered as a shell application.
    ///
    /// \param application the application
    /// \returns `true` if it is a shell application, otherwise `false`.
    bool is_registered(miral::Application const& application) const;

    /// Get a delegate for the given application, or `nullptr` if none exists.
    std::shared_ptr<ShellApplicationDelegate> delegate(miral::Application const& application) const;

private:
    struct ApplicationData
    {
        ShellApplicationRole role;
        ShellApplicationId id;
        std::unique_ptr<ShellApplication> application;
        std::shared_ptr<ShellApplicationDelegate> delegate;
    };

    std::unique_ptr<ShellApplicationSpawner> spawner;
    ShellApplicationId next_id = 0;
    std::vector<ApplicationData> registered_apps;
};
}

#endif // MIRACLE_WM_SHELL_APPLICATION_MANAGER_H