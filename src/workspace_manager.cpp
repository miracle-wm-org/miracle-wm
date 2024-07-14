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

#define MIR_LOG_COMPONENT "workspace_manager"
#include "workspace_manager.h"
#include "output_content.h"
#include "window_helpers.h"
#include <mir/log.h>

using namespace mir::geometry;
using namespace miral;
using namespace miracle;

WorkspaceManager::WorkspaceManager(
    WindowManagerTools const& tools,
    WorkspaceObserverRegistrar& registry,
    std::function<std::shared_ptr<OutputContent> const()> const& get_active_screen) :
    tools_ { tools },
    registry { registry },
    get_active_screen { get_active_screen }
{
}

std::shared_ptr<OutputContent> WorkspaceManager::request_workspace(std::shared_ptr<OutputContent> screen, int key)
{
    if (workspaces[key] != nullptr)
    {
        auto output = workspaces[key];
        auto active_workspace = output->get_active_workspace_num();
        if (active_workspace == key)
        {
            mir::log_warning("Same workspace selected twice in a row");
            return output;
        }

        request_focus(key);
        return output;
    }

    workspaces[key] = screen;
    screen->advise_new_workspace(key);

    request_focus(key);
    registry.advise_created(workspaces[key], key);
    return screen;
}

bool WorkspaceManager::request_first_available_workspace(std::shared_ptr<OutputContent> screen)
{
    for (int i = 1; i < NUM_WORKSPACES; i++)
    {
        if (workspaces[i] == nullptr)
        {
            request_workspace(screen, i);
            return true;
        }
    }

    if (workspaces[0] == nullptr)
    {
        request_workspace(screen, 0);
        return true;
    }

    return false;
}

bool WorkspaceManager::delete_workspace(int key)
{
    if (workspaces[key])
    {
        workspaces[key]->advise_workspace_deleted(key);
        registry.advise_removed(workspaces[key], key);
        workspaces[key] = nullptr;
        return true;
    }

    return false;
}

void WorkspaceManager::request_focus(int key)
{
    if (!workspaces[key])
        return;

    auto active_screen = get_active_screen();
    workspaces[key]->advise_workspace_active(key);

    if (active_screen != nullptr)
    {
        auto active_workspace = active_screen->get_active_workspace_num();
        registry.advise_focused(active_screen, active_workspace, workspaces[key], key);
    }
    else
        registry.advise_focused(nullptr, -1, workspaces[key], key);
}
