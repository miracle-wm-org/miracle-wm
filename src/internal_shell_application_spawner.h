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

#ifndef MIRACLE_WM_INTERNAL_SHELL_APPLICATION_SPAWNER_H
#define MIRACLE_WM_INTERNAL_SHELL_APPLICATION_SPAWNER_H

#include "shell_application_spawner.h"
#include <list>
#include <miral/internal_client.h>

namespace mir
{
class Server;
}

namespace miracle
{
class InternalShellApplicationSpawner : public ShellApplicationSpawner
{
public:
    explicit InternalShellApplicationSpawner(mir::Server& server);
    std::unique_ptr<ShellApplication> spawn(ShellApplicationRole role) override;

private:
    mir::Server& server;

    struct ClientPoolItem
    {
        miral::InternalClientLauncher launcher;
        bool is_taken = false;
    };
    std::list<ClientPoolItem> client_pool;
};

}

#endif // MIRACLE_WM_INTERNAL_SHELL_APPLICATION_SPAWNER_H
