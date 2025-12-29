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

#include "shell_application_manager.h"
#include <algorithm>

miracle::ShellApplicationManager::ShellApplicationManager(std::unique_ptr<ShellApplicationSpawner> spawner) :
    spawner(std::move(spawner))
{
}

miracle::ShellApplicationId miracle::ShellApplicationManager::spawn(ShellApplicationRole type, std::shared_ptr<ShellApplicationDelegate> const& delegate)
{
    registered_apps.push_back({ .role = type,
        .id = next_id++,
        .application = spawner->spawn(type),
        .delegate = std::move(delegate) });
    return registered_apps.back().id;
}

void miracle::ShellApplicationManager::stop(ShellApplicationId app_id)
{
    std::erase_if(
        registered_apps,
        [&app_id](auto const& app)
    {
        if (app.id == app_id)
        {
            if (app.application)
                app.application->stop();
            return true;
        }

        return false;
    });
}

bool miracle::ShellApplicationManager::is_registered(miral::Application const& application) const
{
    return std::ranges::any_of(registered_apps,
        [&application](auto const& app)
    {
        return app.application->application() == application;
    });
}

std::shared_ptr<miracle::ShellApplicationDelegate> miracle::ShellApplicationManager::delegate(miral::Application const& application) const
{
    for (auto const& app : registered_apps)
    {
        if (app.application->application() == application)
            return app.delegate;
    }

    return nullptr;
}
