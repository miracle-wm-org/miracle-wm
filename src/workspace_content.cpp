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
#include "leaf_node.h"
#include "tiling_window_tree.h"
#include "window_helpers.h"
#include "window_metadata.h"
#include <mir/log.h>
#include <mir/scene/surface.h>

using namespace miracle;

WorkspaceContent::WorkspaceContent(
    miracle::OutputContent* screen,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config,
    TilingInterface& node_interface) :
    output { screen },
    tools { tools },
    tree(std::make_shared<TilingWindowTree>(screen, node_interface, config)),
    workspace { workspace }
{
}

int WorkspaceContent::get_workspace() const
{
    return workspace;
}

std::shared_ptr<TilingWindowTree> const& WorkspaceContent::get_tree() const
{
    return tree;
}

void WorkspaceContent::show()
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

        // Pinned windows don't require restoration
        if (metadata->get_is_pinned())
            continue;

        if (auto state = metadata->consume_restore_state())
        {
            miral::WindowSpecification spec;
            spec.state() = state.value();
            tools.modify_window(window, spec);
        }
    }
}

void WorkspaceContent::for_each_window(std::function<void(std::shared_ptr<WindowMetadata>)> const& f)
{
    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (metadata)
            f(metadata);
    }

    tree->foreach_node([&](std::shared_ptr<Node> const& node)
    {
        if (auto leaf = Node::as_leaf(node))
        {
            auto metadata = window_helpers::get_metadata(leaf->get_window(), tools);
            if (metadata)
                f(metadata);
        }
    });
}

void WorkspaceContent::hide()
{
    tree->hide();

    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("hide: floating window lacks metadata");
            continue;
        }

        metadata->set_restore_state(tools.info_for(window).state());
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_hidden;
        tools.modify_window(window, spec);
    }
}

void WorkspaceContent::transfer_pinned_windows_to(std::shared_ptr<WorkspaceContent> const& other)
{
    for (auto it = floating_windows.begin(); it != floating_windows.end();)
    {
        auto metadata = window_helpers::get_metadata(*it, tools);
        if (!metadata)
        {
            mir::log_error("transfer_pinned_windows_to: floating window lacks metadata");
            it++;
            continue;
        }

        if (metadata->get_is_pinned())
        {
            other->add_floating_window(*it);
            it = floating_windows.erase(it);
        }
        else
            it++;
    }
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

std::vector<miral::Window> const& WorkspaceContent::get_floating_windows() const
{
    return floating_windows;
}

OutputContent* WorkspaceContent::get_output()
{
    return output;
}

void WorkspaceContent::trigger_rerender()
{
    // TODO: Ugh, sad. I am forced to set the surface transform so that the surface is rerendered
    for_each_window([&](std::shared_ptr<WindowMetadata> const& metadata)
    {
        auto& window = metadata->get_window();
        auto surface = window.operator std::shared_ptr<mir::scene::Surface>();
        if (surface)
            surface->set_transformation(metadata->get_transform());
    });
}

bool WorkspaceContent::is_empty() const
{
    return tree->is_empty() && floating_windows.empty();
}

int WorkspaceContent::workspace_to_number(int workspace)
{
    if (workspace == 0)
        return 10;

    return workspace - 1;
}