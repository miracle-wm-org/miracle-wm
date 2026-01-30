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

#include "plugin_bridge.h"
#include "container.h"
#include "leaf_container.h"
#include "output_interface.h"
#include "output_manager.h"
#include "parent_container.h"
#include "window_manager_tools_window_controller.h"
#include "workspace_interface.h"
#include <miral/application_info.h>
#include <miral/window_info.h>

using namespace miracle;

namespace
{
miracle_application_info_t from_app_info(miral::ApplicationInfo const& info, miral::WindowSpecification const& spec)
{
    // TODO: Set internal to a unique application ID
    return {
        .application_name = info.name().empty() ? spec.application_id().value_or("").c_str() : info.name().c_str(),
        .internal = 0
    };
}

miracle_application_info_t from_app_info(miral::ApplicationInfo const& info, miral::WindowInfo const& window)
{
    // TODO: Set internal to a unique application ID
    return {
        .application_name = info.name().empty() ? window.application_id().c_str() : info.name().c_str(),
        .internal = 0
    };
}

miracle_workspace_t from_workspace(std::shared_ptr<WorkspaceInterface> const& workspace)
{
    if (workspace == nullptr)
        return { .is_set = false };

    return {
        .is_set = true,
        .has_number = workspace->num().has_value(),
        .number = static_cast<uint32_t>(workspace->num().value_or(0)),
        .has_name = workspace->name().has_value(),
        .num_trees = static_cast<uint32_t>(workspace->trees().size()),
        .internal = reinterpret_cast<uint64_t>(workspace.get())
    };
}

miracle_output_t from_output(std::shared_ptr<OutputInterface> const& output)
{
    auto const area = output->get_area();
    return {
        .position = from_point(area.top_left),
        .size = from_size(area.size),
        .is_primary = output->is_primary(),
        .internal = reinterpret_cast<uint64_t>(output.get())
    };
}

miracle_layout_scheme from_scheme(LayoutScheme scheme)
{
    switch (scheme)
    {
    case LayoutScheme::stacking:
        return miracle_layout_scheme_stacking;
    case LayoutScheme::tabbing:
        return miracle_layout_scheme_tabbed;
    case LayoutScheme::horizontal:
        return miracle_layout_scheme_horizontal;
    case LayoutScheme::vertical:
        return miracle_layout_scheme_vertical;
    case LayoutScheme::none:
    default:
        return miracle_layout_scheme_none;
    }
}

miracle_container_t from_parent(std::shared_ptr<ParentContainer> const& container)
{
    return {
        .type = miracle_container_type_parent,
        .is_floating = !container->anchored(),
        .layout_scheme = from_scheme(container->get_scheme()),
        .num_child_containers = static_cast<uint32_t>(container->num_nodes()),
        .internal = reinterpret_cast<uint64_t>(container.get())
    };
}

miracle_container_t from_child(std::shared_ptr<LeafContainer> const& container)
{
    return {
        .type = miracle_container_type_window,
        .is_floating = 0,
        .layout_scheme = miracle_layout_scheme_none,
        .num_child_containers = 0,
        .internal = reinterpret_cast<uint64_t>(container.get())
    };
}

miracle_window_info_t from_window(miral::WindowInfo const& window_info, uint64_t internal)
{
    return {
        .window_type = window_info.type(),
        .state = window_info.state(),
        .top_left = from_point(window_info.window().top_left()),
        .size = from_size(window_info.window().size()),
        .depth_layer = window_info.depth_layer(),
        .internal = internal
    };
}
}

PluginBridge::PluginBridge(std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<WindowManagerToolsWindowController> const& window_controller) :
    output_manager(output_manager),
    window_controller(window_controller)
{
}

miracle_application_info_t PluginBridge::application(uint64_t window_id)
{
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(reinterpret_cast<void*>(window_id));
    if (std::holds_alternative<miral::WindowSpecification>(plugin_window_info->window_info))
    {
        miral::WindowSpecification spec = std::get<miral::WindowSpecification>(plugin_window_info->window_info);
        return from_app_info(plugin_window_info->app_info, spec);
    }

    miral::Window window = std::get<miral::Window>(plugin_window_info->window_info);
    return from_app_info(plugin_window_info->app_info, window_controller->info_for(window));
}

PluginBridge::WorkspaceResult PluginBridge::workspace(uint64_t window_id)
{
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(reinterpret_cast<void*>(window_id));
    if (std::holds_alternative<miral::Window>(plugin_window_info->window_info))
    {
        miral::WindowInfo const& miral_window_info = window_controller->info_for(std::get<miral::Window>(plugin_window_info->window_info));
        if (auto const container = window_controller->get_container(miral_window_info.window()))
        {
            if (auto const workspace = container->get_workspace())
                return { from_workspace(workspace), workspace->name() };
        }
    }
    else
    {
        if (auto const workspace = output_manager->focused()->active())
            return { from_workspace(workspace), workspace->name() };
    }

    return { from_workspace(nullptr), std::nullopt };
}

miracle_output_t PluginBridge::output(miracle_workspace_t const& workspace)
{
    auto const miracle_workspace = static_cast<WorkspaceInterface*>(reinterpret_cast<void*>(workspace.internal));
    // if (!miracle_workspace)
    // {
    //     return {
    //         .is_set = false,
    //     };
    // }

    return from_output(miracle_workspace->get_output());
}

uint32_t PluginBridge::num_outputs()
{
    return output_manager->outputs().size();
}

PluginBridge::OutputResult PluginBridge::output_at(uint32_t index)
{
    return OutputResult {
        from_output(output_manager->outputs()[index]),
        output_manager->outputs()[index]->name()
    };
}

PluginBridge::OutputResult PluginBridge::output_for_workspace(uint64_t workspace_id)
{
    auto const workspace = static_cast<WorkspaceInterface*>(reinterpret_cast<void*>(workspace_id));
    return OutputResult {
        from_output(workspace->get_output()),
        workspace->get_output()->name()
    };
}

uint32_t PluginBridge::num_workspaces_on_output(miracle_output_t const& output)
{
    auto const miracle_output = static_cast<OutputInterface*>(reinterpret_cast<void*>(output.internal));
    if (!miracle_output)
        return 0;

    return miracle_output->get_workspaces().size();
}

miracle_workspace_t PluginBridge::workspace_on_output_at(miracle_output_t const& output, uint32_t index)
{
    auto const miracle_output = static_cast<OutputInterface*>(reinterpret_cast<void*>(output.internal));
    if (!miracle_output)
        return from_workspace(nullptr);

    return from_workspace(miracle_output->get_workspaces()[index]);
}

miracle_container_t PluginBridge::tree_at_index(uint64_t workspace_id, uint32_t index)
{
    auto const miracle_workspace = static_cast<WorkspaceInterface*>(reinterpret_cast<void*>(workspace_id));
    auto const trees = miracle_workspace->trees();
    return from_parent(trees[index]);
}

miracle_container_t PluginBridge::child_at(uint64_t parent_id, uint32_t index)
{
    auto const parent_container = static_cast<ParentContainer*>(reinterpret_cast<void*>(parent_id));
    auto const child = parent_container->at(index);
    if (child->get_type() == ContainerType::parent)
        return from_parent(Container::as_parent(child));

    return from_child(Container::as_leaf(child));
}

miracle_window_info_t PluginBridge::get_window(miracle_container_t const& container)
{
    // TODO: Expect this to be of type window
    auto const leaf = static_cast<LeafContainer*>(reinterpret_cast<void*>(container.internal));
    miral::Window window = leaf->window().value();
    miral::WindowInfo const& window_info = window_controller->info_for(window);
    miral::ApplicationInfo const& app_info = window_controller->app_info(window);
    auto const plugin_window_info = std::make_shared<PluginWindowInfo>(app_info, window);
    plugin_window_infos.push_back(plugin_window_info);
    return from_window(window_info, reinterpret_cast<uint64_t>(plugin_window_info.get()));
}

PluginBridgeObjectHandle<miracle_window_info_t> PluginBridge::new_window_info(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& spec)
{
    auto const plugin_window_info = std::make_shared<PluginWindowInfo>(app_info, spec);
    plugin_window_infos.push_back(plugin_window_info);
    return PluginBridgeObjectHandle<miracle_window_info_t>({ .window_type = spec.type().value_or(mir_window_type_normal),
                                                               .state = spec.state().value_or(mir_window_state_restored),
                                                               .top_left = from_point(spec.top_left().value_or(geom::Point())),
                                                               .size = from_size(spec.size().value_or(geom::Size(800, 600))),
                                                               .depth_layer = spec.depth_layer().value_or(mir_depth_layer_application),
                                                               .internal = reinterpret_cast<uint64_t>(plugin_window_info.get()) },
        [this, plugin_window_info = plugin_window_info]
    {
        std::erase(plugin_window_infos, plugin_window_info);
    });
}
