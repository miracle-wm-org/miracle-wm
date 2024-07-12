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

#ifndef WORKSPACE_MANAGER_H
#define WORKSPACE_MANAGER_H

#include "workspace_observer.h"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <miral/window_manager_tools.h>
#include <vector>

namespace miracle
{

using miral::Window;
using miral::WindowInfo;
using miral::WindowManagerTools;
using miral::WindowSpecification;
using miral::Workspace;

class OutputContent;

class WorkspaceManager
{
public:
    explicit WorkspaceManager(
        WindowManagerTools const& tools,
        WorkspaceObserverRegistrar& registry,
        std::function<std::shared_ptr<OutputContent> const()> const& get_active_screen);
    virtual ~WorkspaceManager() = default;

    /// Request the workspace. If it does not yet exist, then one
    /// is created on the current Screen. If it does exist, we navigate
    /// to the screen containing that workspace and show it if it
    /// isn't already shown.
    std::shared_ptr<OutputContent> request_workspace(std::shared_ptr<OutputContent> screen, int workspace);

    bool request_first_available_workspace(std::shared_ptr<OutputContent> screen);

    bool delete_workspace(int workspace);

    void request_focus(int workspace);

    static int constexpr NUM_WORKSPACES = 10;
    std::array<std::shared_ptr<OutputContent>, NUM_WORKSPACES> const& get_workspaces() { return workspaces; }

private:
    WindowManagerTools tools_;
    WorkspaceObserverRegistrar& registry;
    std::function<std::shared_ptr<OutputContent> const()> get_active_screen;
    std::array<std::shared_ptr<OutputContent>, NUM_WORKSPACES> workspaces;
};
}

#endif
