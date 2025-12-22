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

void miracle::ShellApplicationManager::register_app(miral::Application const& application, ShellApplicationType type)
{
    switch (type)
    {
    case ShellApplicationType::parent_container_background:
        parent_container_background_apps.push_back(application);
        break;
    }
}

void miracle::ShellApplicationManager::unregister_app(miral::Application const& application, ShellApplicationType type)
{
    switch (type)
    {
    case ShellApplicationType::parent_container_background:
        std::erase_if(
            parent_container_background_apps,
            [&application](miral::Application const& app)
        {
            return app == application;
        });
        break;
    }
}

bool miracle::ShellApplicationManager::is_registered(miral::Application const& application, ShellApplicationType type) const
{
    switch (type)
    {
    case ShellApplicationType::parent_container_background:
        return std::ranges::any_of(parent_container_background_apps,
            [&application](miral::Application const& app)
        {
            return app == application;
        });
    default:
        return false;
    }
}