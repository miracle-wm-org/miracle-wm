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

class Output;

class WorkspaceManager
{
public:
    explicit WorkspaceManager(
        WindowManagerTools const& tools,
        WorkspaceObserverRegistrar& registry,
        std::function<Output const*()> const& get_active_screen);
    virtual ~WorkspaceManager() = default;

    /// Request the workspace. If it does not yet exist, then one
    /// is created on the current Screen. If it does exist, we navigate
    /// to the screen containing that workspace and show it if it
    /// isn't already shown.
    std::shared_ptr<Output> request_workspace(std::shared_ptr<Output> const& screen, int workspace);

    int request_first_available_workspace(std::shared_ptr<Output> const& screen);

    /// Request the workspace by name. If it does not exist, then it will not
    /// be selected.
    bool request_workspace(std::string const& name);

    /// Selects the next workspace after the current selected one.
    bool request_next(std::shared_ptr<Output> const& output);

    /// Selects the workspace before the current selected one
    bool request_prev(std::shared_ptr<Output> const& output);

    bool request_next_on_output(Output const&);

    bool request_prev_on_output(Output const&);

    bool delete_workspace(int workspace);

    void request_focus(int workspace);

    static int constexpr NUM_WORKSPACES = 10;
    std::array<std::shared_ptr<Output>, NUM_WORKSPACES> const& get_output_to_workspace_mapping() { return output_to_workspace_mapping; }

private:
    WindowManagerTools tools_;
    WorkspaceObserverRegistrar& registry;
    std::function<Output const*()> get_active_screen;
    std::array<std::shared_ptr<Output>, NUM_WORKSPACES> output_to_workspace_mapping;
    int last_selected_workspace = -1;
};
}

#endif
