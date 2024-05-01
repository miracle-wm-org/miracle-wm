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

#define MIR_LOG_COMPONENT "workspace_content"

#include "workspace_content.h"
#include "tiling_window_tree.h"
#include "window_helpers.h"
#include "window_metadata.h"
#include <mir/log.h>

using namespace miracle;

WorkspaceContent::WorkspaceContent(
    miracle::OutputContent* screen,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config,
    TilingInterface& node_interface) :
    tools { tools },
    tree(std::make_shared<TilingWindowTree>(screen, node_interface, config)),
    workspace { workspace }
{
}

int WorkspaceContent::get_workspace() const
{
    return workspace;
}

std::shared_ptr<TilingWindowTree> WorkspaceContent::get_tree() const
{
    return tree;
}

void WorkspaceContent::show(std::vector<std::shared_ptr<WindowMetadata>> const& pinned_windows)
{
    tree->show();

    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("show: floating window lacks metadata");
            continue;
        }

        miral::WindowSpecification spec;
        spec.state() = metadata->consume_restore_state();
        tools.modify_window(window, spec);
    }

    for (auto const& metadata : pinned_windows)
    {
        floating_windows.push_back(metadata->get_window());
    }
}

std::vector<std::shared_ptr<WindowMetadata>> WorkspaceContent::hide()
{
    tree->hide();

    std::vector<std::shared_ptr<WindowMetadata>> pinned_windows;
    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("hide: floating window lacks metadata");
            continue;
        }

        if (metadata->get_is_pinned())
        {
            pinned_windows.push_back(metadata);
            break;
        }

        metadata->set_restore_state(tools.info_for(window).state());
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_hidden;
        tools.modify_window(window, spec);
    }

    floating_windows.erase(std::remove_if(floating_windows.begin(), floating_windows.end(), [&](const auto& x)
    {
        auto metadata = window_helpers::get_metadata(x, tools);
        return std::find(pinned_windows.begin(), pinned_windows.end(), metadata) != pinned_windows.end();
    }),
        floating_windows.end());

    return pinned_windows;
}

bool WorkspaceContent::has_floating_window(miral::Window const& window)
{
    for (auto const& other : floating_windows)
    {
        if (other == window)
            return true;
    }

    return false;
}

void WorkspaceContent::add_floating_window(miral::Window const& window)
{
    floating_windows.push_back(window);
}

void WorkspaceContent::remove_floating_window(miral::Window const& window)
{
    floating_windows.erase(std::remove(floating_windows.begin(), floating_windows.end(), window));
}