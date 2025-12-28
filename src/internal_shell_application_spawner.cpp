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

#define MIR_LOG_COMPONENT "internal_shell_application_spawner"

#include "internal_shell_application_spawner.h"
#include "parent_background_internal_client.h"
#include <mir/log.h>

using namespace miracle;

InternalShellApplicationSpawner::InternalShellApplicationSpawner(miral::InternalClientLauncher const& launcher) :
    launcher(launcher)
{
}

std::unique_ptr<ShellApplication> InternalShellApplicationSpawner::spawn(ShellApplicationRole role)
{
    switch (role)
    {
    case ShellApplicationRole::parent_container_background:
    {
        auto background_client = std::make_unique<ParentBackgroundInternalClient>();
        launcher.launch(*background_client);
        return background_client;
    }
    }

    mir::log_error("Cannot spawn a shell application with an unknown role");
    return nullptr;
}
