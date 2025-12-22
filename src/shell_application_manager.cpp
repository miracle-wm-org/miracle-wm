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

void miracle::ShellApplicationManager::register_app(miral::Application const& application, ShellApplicationType type, std::shared_ptr<ShellComponentDelegate> const& delegate)
{
    switch (type)
    {
    case ShellApplicationType::parent_container_background:
        registered_apps.push_back({
            .type = type,
            .application = application,
            .delegate = std::move(delegate)
        });
        break;
    }
}

void miracle::ShellApplicationManager::unregister_app(miral::Application const& application)
{
    std::erase_if(
        registered_apps,
        [&application](auto const& app)
    {
        return app.application == application;
    });
}

bool miracle::ShellApplicationManager::is_registered(miral::Application const& application) const
{
    return std::ranges::any_of(registered_apps,
        [&application](auto const& app)
    {
        return app.application == application;
    });
}

std::shared_ptr<miracle::ShellComponentDelegate> miracle::ShellApplicationManager::delegate(miral::Application const& application) const
{
    for (auto const& app : registered_apps)
    {
        if (app.application == application)
            return app.delegate;
    }

    return nullptr;
}
