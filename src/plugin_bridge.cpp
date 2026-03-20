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

#include "plugin_bridge.h"
#include "abstract_output.h"
#include "abstract_workspace.h"
#include "animator.h"
#include "compositor_state.h"
#include "container.h"
#include "leaf_container.h"
#include "output_manager.h"
#include "parent_container.h"
#include "plugin_manager.h"
#include "window_container.h"
#include "window_controller.h"
#include "workspace_manager.h"
#include <glm/gtc/type_ptr.hpp>
#include <mir/server_action_queue.h>
#include <miral/application_info.h>
#include <miral/window_info.h>

using namespace miracle;

namespace miracle
{
miracle_workspace_t from_workspace(std::shared_ptr<AbstractWorkspace> const& workspace)
{
    if (workspace == nullptr)
        return { .is_set = false };

    auto const rect = workspace->area();
    return {
        .is_set = true,
        .has_number = workspace->num().has_value(),
        .number = static_cast<uint32_t>(workspace->num().value_or(0)),
        .has_name = workspace->name().has_value(),
        .num_trees = static_cast<uint32_t>(workspace->trees().size()),
        .internal = static_cast<uint64_t>(workspace->id()),
        .position = from_point(rect.top_left),
        .size = from_size(rect.size),
    };
}

miracle_window_info_t from_window(miral::WindowInfo const& window_info, uint64_t internal, WindowContainer* container)
{
    glm::mat4 transform(1.f);
    float alpha = 1.f;
    if (container)
    {
        transform = container->get_window_transform();
        alpha = container->get_window_alpha();
    }
    miracle_window_info_t result {
        .window_type = window_info.type(),
        .state = window_info.state(),
        .top_left = from_point(window_info.window().top_left()),
        .size = from_size(window_info.window().size()),
        .depth_layer = window_info.depth_layer(),
        .internal = internal,
        .alpha = alpha
    };
    std::memcpy(result.transform, glm::value_ptr(transform), sizeof(float) * 16);
    return result;
}
}

namespace
{
miracle_application_info_t from_app_info(miral::ApplicationInfo const& info, miral::WindowSpecification const& spec, uint64_t internal)
{
    return {
        .application_name = info.name().empty() ? spec.application_id().value_or("").c_str() : info.name().c_str(),
        .internal = internal
    };
}

miracle_application_info_t from_app_info(miral::ApplicationInfo const& info, miral::WindowInfo const& window, uint64_t internal)
{
    return {
        .application_name = info.name().empty() ? window.application_id().c_str() : info.name().c_str(),
        .internal = internal
    };
}

miracle_output_t from_output(std::shared_ptr<AbstractOutput> const& output)
{
    auto const area = output->get_area();
    return {
        .position = from_point(area.top_left),
        .size = from_size(area.size),
        .is_primary = output->is_primary(),
        .num_workspaces = static_cast<uint32_t>(output->get_workspaces().size()),
        .internal = static_cast<uint64_t>(output->id())
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
        .layout_scheme = static_cast<uint32_t>(from_scheme(container->get_scheme())),
        .num_child_containers = static_cast<uint32_t>(container->num_children()),
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

}

PluginBridge::PluginBridge(std::shared_ptr<OutputManager> const& output_manager,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<WorkspaceManager> const& workspace_manager,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<WindowIdMap> const& window_id_map,
    std::shared_ptr<ApplicationIdMap> const& application_id_map,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue) :
    output_manager(output_manager),
    window_controller(window_controller),
    workspace_manager(workspace_manager),
    compositor_state(compositor_state),
    window_id_map(window_id_map),
    application_id_map(application_id_map),
    animator(animator),
    server_action_queue(server_action_queue)
{
}

uint64_t PluginBridge::find_window_id(miral::Window const& window) const
{
    for (auto const& [id, w] : *window_id_map)
        if (w == window)
            return id;
    return 0;
}

uint64_t PluginBridge::find_application_id(miral::Application const& app) const
{
    for (auto const& [id, a] : *application_id_map)
        if (a == app)
            return id;
    return 0;
}

std::shared_ptr<AbstractOutput> PluginBridge::resolve_output(uint64_t output_id)
{
    for (auto const& output : output_manager->outputs())
        if (static_cast<uint64_t>(output->id()) == output_id)
            return output;
    return nullptr;
}

std::shared_ptr<AbstractWorkspace> PluginBridge::resolve_workspace_shared(uint64_t workspace_id)
{
    for (auto const& output : output_manager->outputs())
        for (auto const& ws : output->get_workspaces())
            if (static_cast<uint64_t>(ws->id()) == workspace_id)
                return ws;
    return nullptr;
}

miracle_application_info_t PluginBridge::application_from_window(uint64_t window_id)
{
    // Check the stable window map first (post-creation windows).
    auto it = window_id_map->find(window_id);
    if (it != window_id_map->end())
    {
        auto const& window = it->second;
        uint64_t const app_id = find_application_id(window.application());
        return from_app_info(window_controller->app_info(window), window_controller->info_for(window), app_id);
    }

    // Fall back to the PluginWindowInfo pointer for pre-creation (WindowSpecification) case.
    auto const plugin_window_info = static_cast<PluginWindowInfo*>(reinterpret_cast<void*>(window_id));
    for (auto const& info : plugin_window_infos)
    {
        if (info.get() == plugin_window_info)
        {
            miral::WindowSpecification spec = std::get<miral::WindowSpecification>(plugin_window_info->window_info);
            uint64_t const app_id = find_application_id(plugin_window_info->app_info.application());
            return from_app_info(plugin_window_info->app_info, spec, app_id);
        }
    }

    return {};
}

PluginBridge::WorkspaceResult PluginBridge::workspace_from_window(uint64_t window_id)
{
    // Check the stable window map first (post-creation windows).
    auto it = window_id_map->find(window_id);
    if (it != window_id_map->end())
    {
        miral::WindowInfo const& miral_window_info = window_controller->info_for(it->second);
        if (auto const container = window_controller->get_window_container(miral_window_info.window()))
        {
            if (auto const workspace = container->get_workspace())
                return { from_workspace(workspace), workspace->name() };
        }
        return { from_workspace(nullptr), std::nullopt };
    }

    // Fall back: pre-creation window uses the active workspace.
    if (auto const workspace = output_manager->focused()->active())
        return { from_workspace(workspace), workspace->name() };

    return { from_workspace(nullptr), std::nullopt };
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

PluginBridge::OutputResult PluginBridge::output_from_workspace(uint64_t workspace_id)
{
    auto const workspace = resolve_workspace_shared(workspace_id);
    return OutputResult {
        from_output(workspace->get_output()),
        workspace->get_output()->name()
    };
}

uint32_t PluginBridge::num_workspaces_on_output(uint64_t output_id)
{
    auto const output = resolve_output(output_id);
    return output->get_workspaces().size();
}

PluginBridge::WorkspaceResult PluginBridge::workspace_on_output_at_index(uint64_t output_id, uint32_t index)
{
    auto const output = resolve_output(output_id);
    auto const workspace = output->get_workspaces()[index];
    return { from_workspace(workspace), workspace->name() };
}

miracle_container_t PluginBridge::tree_at_index(uint64_t workspace_id, uint32_t index)
{
    auto const workspace = resolve_workspace_shared(workspace_id);
    auto const trees = workspace->trees();
    return from_parent(trees[index]);
}

miracle_container_t PluginBridge::container_from_window(uint64_t window_id)
{
    auto it = window_id_map->find(window_id);
    if (it == window_id_map->end())
        return {};

    auto const container = window_controller->get_window_container(it->second);
    if (!container)
        return {};

    return from_child(std::static_pointer_cast<LeafContainer>(container));
}

miracle_container_t PluginBridge::parent_from_container(uint64_t container_id)
{
    auto const container = resolve_container(container_id);
    if (!container)
        return {};

    auto const parent = container->get_parent().lock();
    if (!parent)
        return {};

    return from_parent(parent);
}

miracle_container_t PluginBridge::child_at(uint64_t parent_id, uint32_t index)
{
    auto const parent_container = static_cast<ParentContainer*>(reinterpret_cast<void*>(parent_id));
    auto const child = parent_container->at(index);
    if (auto const parent = Container::as_parent(child))
        return from_parent(parent);

    return from_child(Container::as_leaf(child));
}

PluginBridge::WindowResult PluginBridge::get_window(uint64_t container_address)
{
    // TODO: Expect this to be of type window
    auto const leaf = static_cast<LeafContainer*>(reinterpret_cast<void*>(container_address));
    miral::Window window = leaf->window().value();
    miral::WindowInfo const& window_info = window_controller->info_for(window);
    return {
        from_window(window_info, find_window_id(window), window_controller->get_window_container(window).get()),
        window_info.name()
    };
}

PluginBridgeObjectHandle<miracle_window_info_t> PluginBridge::new_window_info(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& spec, uint64_t window_id)
{
    auto const plugin_window_info = std::make_shared<PluginWindowInfo>(app_info, spec);
    plugin_window_infos.push_back(plugin_window_info);
    miracle_window_info_t info {
        .window_type = spec.type().value_or(mir_window_type_normal),
        .state = spec.state().value_or(mir_window_state_restored),
        .top_left = from_point(spec.top_left().value_or(geom::Point())),
        .size = from_size(spec.size().value_or(geom::Size(800, 600))),
        .depth_layer = spec.depth_layer().value_or(mir_depth_layer_application),
        .internal = window_id,
        .alpha = 1.f
    };
    glm::mat4 identity(1.f);
    std::memcpy(info.transform, glm::value_ptr(identity), sizeof(float) * 16);
    return PluginBridgeObjectHandle<miracle_window_info_t>(std::move(info),
        [this, plugin_window_info = plugin_window_info]
    {
        std::erase(plugin_window_infos, plugin_window_info);
    });
}

PluginBridgeObjectHandle<miracle_window_info_t> PluginBridge::existing_window_info(miral::WindowInfo const& window_info)
{
    uint64_t const stable_id = find_window_id(window_info.window());
    return PluginBridgeObjectHandle<miracle_window_info_t>(
        from_window(window_info, stable_id, window_controller->get_window_container(window_info.window()).get()),
        // clang-format off
        [] {});
    // clang-format on
}

Container* PluginBridge::resolve_container(uint64_t container_internal)
{
    return static_cast<Container*>(reinterpret_cast<void*>(container_internal));
}

PluginBridge::WorkspaceResult PluginBridge::request_workspace(std::optional<int> num, std::optional<std::string> name, bool focus)
{
    auto* output = output_manager->focused().get();
    auto* result = workspace_manager->request_workspace(output, num, name, focus);
    if (!result)
        return { from_workspace(nullptr), std::nullopt };

    for (auto const& w : result->get_output()->get_workspaces())
    {
        if (w.get() == result)
            return { from_workspace(w), w->name() };
    }

    return { from_workspace(nullptr), std::nullopt };
}

PluginBridge::WorkspaceResult PluginBridge::active_workspace()
{
    auto const focused = output_manager->focused();
    if (!focused)
        return { from_workspace(nullptr), std::nullopt };

    auto const workspace = focused->active();
    if (!workspace)
        return { from_workspace(nullptr), std::nullopt };

    return { from_workspace(workspace), workspace->name() };
}

AbstractWorkspace* PluginBridge::resolve_workspace(uint64_t workspace_internal)
{
    return resolve_workspace_shared(workspace_internal).get();
}

PluginBridge::WorkspaceResult PluginBridge::workspace_by_id(uint32_t id)
{
    auto const ws = resolve_workspace_shared(static_cast<uint64_t>(id));
    if (!ws)
        return { from_workspace(nullptr), std::nullopt };
    return { from_workspace(ws), ws->name() };
}

uint32_t PluginBridge::num_managed_windows(uint32_t plugin_handle)
{
    uint32_t count = 0;
    auto const lock = compositor_state->lock();
    for (auto const& weak_container : compositor_state->windows())
    {
        auto const container = weak_container.lock();
        if (!container)
            continue;

        auto const handle = container->plugin_handle();
        if (!handle.has_value() || handle.value() != plugin_handle)
            continue;

        if (!container->window().has_value())
            continue;

        count++;
    }

    return count;
}

PluginBridge::WindowResult PluginBridge::get_managed_window_at(uint32_t plugin_handle, uint32_t index)
{
    uint32_t count = 0;
    auto const lock = compositor_state->lock();
    for (auto const& weak_container : compositor_state->windows())
    {
        auto const container = weak_container.lock();
        if (!container)
            continue;

        auto const handle = container->plugin_handle();
        if (!handle.has_value() || handle.value() != plugin_handle)
            continue;

        auto const window = container->window();
        if (!window.has_value())
            continue;

        if (count == index)
        {
            miral::WindowInfo const& window_info = window_controller->info_for(window.value());
            return {
                from_window(window_info, find_window_id(window.value()), window_controller->get_window_container(window.value()).get()),
                window_info.name()
            };
        }

        count++;
    }

    return { miracle_window_info_t {}, "" };
}

int32_t PluginBridge::queue_custom_animation(
    PluginHandle plugin_handle,
    uint32_t* out_animation_id,
    PluginManager* manager,
    float duration_seconds)
{
    uint32_t const animation_id = next_animation_id++;
    *out_animation_id = animation_id;

    auto const handle = animator->register_animateable();

    auto saq = server_action_queue;
    auto on_tick = [plugin_handle, animation_id, manager, duration_seconds, saq, compositor_state = compositor_state,
                       elapsed = 0.0f](float dt) mutable -> bool
    {
        elapsed += dt;
        saq->enqueue(manager, [plugin_handle, animation_id, dt, elapsed, manager, compositor_state = compositor_state]
        {
            // TODO: This lock is SUCH a hammer, we need to fix this!
            auto const lock = compositor_state->lock();
            manager->custom_animate(plugin_handle, animation_id, dt, elapsed);
        });
        return elapsed >= duration_seconds;
    };

    animator->append(CustomAnimation(handle, std::move(on_tick)));
    return 0;
}

int32_t PluginBridge::window_set_state(uint64_t window_internal, int32_t state)
{
    auto it = window_id_map->find(window_internal);
    if (it == window_id_map->end())
        return -1;

    window_controller->change_state(it->second, static_cast<MirWindowState>(state));
    return 0;
}

int32_t PluginBridge::window_set_workspace(uint64_t window_internal, uint64_t workspace_internal)
{
    auto wit = window_id_map->find(window_internal);
    if (wit == window_id_map->end())
        return -1;

    auto const container = window_controller->get_window_container(wit->second);
    if (!container)
        return -1;

    auto const new_ws = resolve_workspace_shared(workspace_internal);
    if (!new_ws)
        return -1;

    if (auto const old_ws = container->get_workspace())
        old_ws->remove_other_container(container);

    new_ws->add_other_container(container);
    return 0;
}

int32_t PluginBridge::window_set_rectangle(uint64_t window_internal, int32_t x, int32_t y, int32_t width, int32_t height, bool animate)
{
    auto it = window_id_map->find(window_internal);
    if (it == window_id_map->end())
        return -1;

    auto const& window = it->second;
    auto const container = window_controller->get_window_container(window);
    if (!container)
        return -1;

    geom::Rectangle const old_rect = container->get_logical_area();
    geom::Rectangle const new_rect {
        geom::Point { x,     y      },
        geom::Size { width, height }
    };
    window_controller->set_rectangle(window, old_rect, new_rect, animate);
    return 0;
}

int32_t PluginBridge::window_set_transform(uint64_t window_internal, float const* transform)
{
    auto it = window_id_map->find(window_internal);
    if (it == window_id_map->end())
        return -1;

    auto const container = window_controller->get_window_container(it->second);
    if (!container)
        return -1;

    glm::mat4 mat;
    std::memcpy(glm::value_ptr(mat), transform, sizeof(float) * 16);
    container->set_window_transform(mat);
    return 0;
}

int32_t PluginBridge::window_set_alpha(uint64_t window_internal, float alpha)
{
    auto it = window_id_map->find(window_internal);
    if (it == window_id_map->end())
        return -1;

    auto const container = window_controller->get_window_container(it->second);
    if (!container)
        return -1;

    container->set_window_alpha(alpha);
    return 0;
}

int32_t PluginBridge::window_request_focus(uint64_t window_internal)
{
    auto it = window_id_map->find(window_internal);
    if (it == window_id_map->end())
        return -1;

    window_controller->select_active_window(it->second);
    return 0;
}

void PluginBridge::set_plugin_userdata(uint32_t handle, std::string const& userdata_json)
{
    plugin_userdata_map[handle] = userdata_json;
}

std::string const* PluginBridge::get_plugin_userdata(uint32_t handle) const
{
    auto const it = plugin_userdata_map.find(handle);
    return it != plugin_userdata_map.end() ? &it->second : nullptr;
}
