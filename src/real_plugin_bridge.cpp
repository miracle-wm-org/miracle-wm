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

#include "real_plugin_bridge.h"
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


miracle_application_info_t from_app_info(miral::ApplicationInfo const& info)
{
    return {
        .application_name = info.name().c_str()
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
        .name = workspace->name().value_or("").c_str(),
        .num_trees = static_cast<uint32_t>(workspace->trees().size()),
        .internal = static_cast<void*>(workspace.get())
    };
}

miracle_output_t from_output(std::shared_ptr<OutputInterface> const& output)
{
    if (output == nullptr)
        return { .is_set = false };

    auto const area = output->get_area();
    return {
        .is_set = true,
        .position = from_point(area.top_left),
        .size = from_size(area.size),
        .name = output->name().c_str(),
        .is_primary = output->is_primary(),
        .internal = static_cast<void*>(output.get())
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
        .internal = static_cast<void*>(container.get())
    };
}

miracle_container_t from_child(std::shared_ptr<LeafContainer> const& container)
{
    return {
        .type = miracle_container_type_window,
        .is_floating = 0,
        .layout_scheme = miracle_layout_scheme_none,
        .num_child_containers = 0,
        .internal = static_cast<void*>(container.get())
    };
}

miracle_window_info_t from_window(miral::WindowInfo const& window_info, void* internal)
{
    return {
        .window_type = window_info.type(),
        .state = window_info.state(),
        .top_left = from_point(window_info.window().top_left()),
        .size = from_size(window_info.window().size()),
        .title = window_info.name().c_str(),
        .depth_layer = window_info.depth_layer(),
        .internal = internal
    };
}
}

RealPluginBridge::RealPluginBridge(std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<WindowManagerToolsWindowController> const& window_controller) :
    output_manager(output_manager),
    window_controller(window_controller)
{
}

miracle_application_info_t RealPluginBridge::application(miracle_window_info_t const& window_info)
{
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(window_info.internal);
    return from_app_info(plugin_window_info->app_info);
}

miracle_workspace_t RealPluginBridge::workspace(miracle_window_info_t const& window_info)
{
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(window_info.internal);
    if (std::holds_alternative<miral::Window>(plugin_window_info->window_info))
    {
        miral::WindowInfo const& miral_window_info = window_controller->info_for(std::get<miral::Window>(plugin_window_info->window_info));
        if (auto const container = window_controller->get_container(miral_window_info.window()))
            return from_workspace(container->get_workspace());
    }

    return from_workspace(nullptr);
}

miracle_output_t RealPluginBridge::output(miracle_workspace_t const& workspace)
{
    auto const miracle_workspace = static_cast<WorkspaceInterface*>(workspace.internal);
    if (!miracle_workspace)
    {
        return {
            .is_set = false,
        };
    }

    return from_output(miracle_workspace->get_output());
}

uint32_t RealPluginBridge::num_outputs()
{
    return output_manager->outputs().size();
}

miracle_output_t RealPluginBridge::output_at(uint32_t index)
{
    return from_output(output_manager->outputs()[index]);
}

uint32_t RealPluginBridge::num_workspaces_on_output(miracle_output_t const& output)
{
    auto const miracle_output = static_cast<OutputInterface*>(output.internal);
    if (!miracle_output)
        return 0;

    return miracle_output->get_workspaces().size();
}

miracle_workspace_t RealPluginBridge::workspace_on_output_at(miracle_output_t const& output, uint32_t index)
{
    auto const miracle_output = static_cast<OutputInterface*>(output.internal);
    if (!miracle_output)
        return from_workspace(nullptr);

    return from_workspace(miracle_output->get_workspaces()[index]);
}

miracle_container_t RealPluginBridge::tree_at_index(miracle_workspace_t const& workspace, uint32_t index)
{
    auto const miracle_workspace = static_cast<WorkspaceInterface*>(workspace.internal);
    auto const trees = miracle_workspace->trees();
    return from_parent(trees[index]);
}

miracle_container_t RealPluginBridge::child_at(miracle_container_t const& parent, uint32_t index)
{
    auto const parent_container = static_cast<ParentContainer*>(parent.internal);
    auto const child = parent_container->at(index);
    if (child->get_type() == ContainerType::parent)
        return from_parent(Container::as_parent(child));

    return from_child(Container::as_leaf(child));
}

miracle_window_info_t RealPluginBridge::get_window(miracle_container_t const& container)
{
    // TODO: Expect this to be of type window
    auto const leaf = static_cast<LeafContainer*>(container.internal);
    miral::Window window = leaf->window().value();
    miral::WindowInfo const& window_info = window_controller->info_for(window);
    miral::ApplicationInfo const& app_info = window_controller->app_info(window);
    auto const plugin_window_info = std::make_shared<PluginWindowInfo>(app_info, window);
    plugin_window_infos.push_back(plugin_window_info);
    return from_window(window_info, plugin_window_info.get());
}

miracle_window_info_t RealPluginBridge::new_window_info(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& spec)
{
    auto const plugin_window_info = std::make_shared<PluginWindowInfo>(app_info, spec);
    plugin_window_infos.push_back(plugin_window_info);
    return {
        .window_type = spec.type().value_or(mir_window_type_normal),
        .state = spec.state().value_or(mir_window_state_restored),
        .top_left = from_point(spec.top_left().value_or(geom::Point())),
        .size = from_size(spec.size().value_or(geom::Size(800, 600))),
        .title = spec.name().value_or("").c_str(),
        .depth_layer = spec.depth_layer().value_or(mir_depth_layer_application),
        .internal = plugin_window_info.get()
    };
}
