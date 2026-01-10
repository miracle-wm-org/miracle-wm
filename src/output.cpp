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

#define MIR_LOG_COMPONENT "output_content"
#define GLM_ENABLE_EXPERIMENTAL

#include "output.h"
#include "animator.h"
#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "mir_version_manager.h"
#include "vector_helpers.h"
#include "workspace.h"
#include "workspace_manager.h"
#include <glm/gtx/transform.hpp>
#include <memory>
#include <mir/log.h>
#include <miral/application_info.h>
#include <miral/window_info.h>
#include <miral/zone.h>

#include "shell_application_manager.h"

using namespace miracle;
namespace mg = mir::graphics;

Output::Output(
    std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
    std::string name,
    int id,
    geom::Rectangle const& area,
    OutputConfigDetails const& output_config,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
    std::shared_ptr<PluginManager> const& plugin_manager) :
    shell_application_manager { shell_application_manager },
    name_ { std::move(name) },
    id_ { id },
    area { area },
    output_config { output_config },
    state { state },
    config { config },
    window_controller { window_controller },
    animator { animator },
    server_action_queue { server_action_queue },
    plugin_manager { plugin_manager }
{
}

Output::~Output() = default;

std::shared_ptr<WorkspaceInterface> Output::active() const
{
    if (active_workspace.expired())
        return nullptr;

    return active_workspace.lock();
}

std::shared_ptr<Container> Output::intersect(float x, float y)
{
    // Intersect a window. If the window is on the currently active workspace
    // or the window is a shell component, then return it.
    auto const window = window_controller->window_at(x, y);
    if (auto const result = window_controller->get_container(window))
    {
        if (result->get_workspace() == active() || result->get_type() == ContainerType::shell)
            return result;
    }

    return nullptr;
}

std::shared_ptr<Container> Output::intersect_leaf(float x, float y, bool ignore_selected)
{
    if (active_workspace.expired())
    {
        mir::log_error("intersect_leaf: there is no active workspace");
        return nullptr;
    }

    auto const workspace = active_workspace.lock().get();
    std::shared_ptr<Container> result = nullptr;
    workspace->for_each_window([&](std::shared_ptr<Container> const& container)
    {
        if (ignore_selected && container == state->focused_container())
            return false;

        if (container->get_type() != ContainerType::leaf)
            return false;

        if (container->get_visible_area().contains(geom::Point(x, y)))
        {
            result = container;
            return true;
        }

        return false;
    });

    return result;
}

AllocationHint Output::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    AllocationHint hint)
{
    if (shell_application_manager->is_registered(app_info.application()))
    {
        hint.container_type = ContainerType::shell;
        return hint;
    }

    auto const has_exclusive_rect = requested_specification.exclusive_rect().is_set();
    auto const is_attached = requested_specification.attached_edges().is_set();
    auto const wrong_leaf_state = requested_specification.state() == mir_window_state_hidden
        || requested_specification.state() == mir_window_state_attached;
    if (has_exclusive_rect || is_attached || wrong_leaf_state)
        hint.container_type = ContainerType::shell;
    else
    {
        auto const t = requested_specification.type();
        if (t == mir_window_type_normal || t == mir_window_type_freestyle)
            hint.container_type = ContainerType::leaf;
        else
            hint.container_type = ContainerType::shell; // This is probably a tooltip or something
    }

    if (hint.container_type == ContainerType::shell)
        return hint;

    return active()->allocate_position(app_info, requested_specification, hint);
}

std::shared_ptr<Container> Output::create_container(
    miral::WindowInfo const& window_info, AllocationHint const& hint) const
{
    return active()->create_container(window_info, hint);
}

void Output::delete_container(std::shared_ptr<Container> const& container)
{
    auto const workspace = container->get_workspace();
    if (!workspace)
        return;

    workspace->delete_container(container);
}

void Output::insert_workspace_sorted(std::shared_ptr<WorkspaceInterface> const& new_workspace)
{
    insert_sorted(workspaces, new_workspace, [](std::shared_ptr<WorkspaceInterface> const& a, std::shared_ptr<WorkspaceInterface> const& b)
    {
        if (a->num() && b->num())
            return a->num().value() < b->num().value();
        else if (a->num())
            return true;
        else if (b->num())
            return false;
        else
            return false;
    });
}

void Output::advise_new_workspace(WorkspaceCreationData const&& data)
{
    // Workspaces are always kept in sorted order with numbered workspaces in front followed by all other workspaces
    auto const new_workspace = std::make_shared<Workspace>(
        shell_application_manager,
        shared_from_this(),
        data.id,
        data.num,
        data.name,
        config,
        window_controller,
        state,
        data.registrar,
        animator,
        server_action_queue,
        plugin_manager);
    insert_workspace_sorted(new_workspace);
}

void Output::advise_workspace_deleted(WorkspaceManager& workspace_manager, uint32_t id)
{
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (it->get()->id() == id)
        {
            workspaces.erase(it);
            return;
        }
    }
}

void Output::move_workspace_to(WorkspaceManager& workspace_manager, WorkspaceInterface* workspace)
{
    if (workspace->get_output().get() == this)
        return;

    // Remove the workspace from its old output. Note that we grab a reference to it
    // so that its reference count doesn't erroneously hit zero.
    std::shared_ptr<WorkspaceInterface> to_add = nullptr;
    auto const old_output = workspace->get_output();
    if (!old_output)
    {
        mir::log_error("Failed to find the old output");
        return;
    }

    for (auto const& w : old_output->get_workspaces())
    {
        if (w->id() == workspace->id())
        {
            to_add = w;
            old_output->advise_workspace_deleted(workspace_manager, workspace->id());
            break;
        }
    }

    if (to_add == nullptr)
    {
        mir::log_error("Failed to find the old workspace!");
        return;
    }

    // Next, move the workspace to this output and remove it focus it
    mir::log_info("Moving workspace %d to output %d", workspace->id(), id_);
    insert_workspace_sorted(to_add);
    to_add->set_output(shared_from_this());
    workspace_manager.request_focus(to_add->id());

    // Finally, if [old_output] has no workspaces left, we need to request a workspace on it
    if (old_output->get_workspaces().empty())
        workspace_manager.request_first_available_workspace(old_output.get());
}

bool Output::advise_workspace_active(WorkspaceManager& workspace_manager, uint32_t id)
{
    // When the workspace becomes active, we try to move this output to the new
    // workspace transform, which may involve an animation.
    mir::log_info("advise_workspace_active: %d", id);

    // First, we find where we're coming from and where we're going to.
    std::shared_ptr<WorkspaceInterface> from = nullptr;
    std::shared_ptr<WorkspaceInterface> to = nullptr;
    size_t from_index = 0;
    size_t to_index = 0;
    for (size_t i = 0; i < workspaces.size(); i++)
    {
        auto const& workspace = workspaces[i];
        if (!active_workspace.expired() && workspace == active_workspace.lock())
        {
            from = workspace;
            from_index = i;
        }

        if (workspace->id() == id)
        {
            to = workspace;
            to_index = i;
        }
    }

    if (!to)
    {
        mir::log_error("Output::advise_workspace_active: switch to workspace that doesn't exist: %d", id);
        return false;
    }

    // If we're not coming from anywhere, then we can immediately jump to our destination.
    if (!from || to == from)
    {
        active_workspace = to;
        to->show(geom::Point(0, 0));
        return true;
    }

    // It is very important that [active_workspace] be modified before notifications are sent out.
    active_workspace = to;

    from->transfer_pinned_windows_to(to);
    auto const from_end = to_index > from_index ? geom::Point(-area.size.width.as_int(), 0) : geom::Point(area.size.width.as_int(), 0);
    auto const to_start = to_index > from_index ? geom::Point(area.size.width.as_int(), 0) : geom::Point(-area.size.width.as_int(), 0);
    to->show(to_start);

    // If [from] is empty, we can delete the workspace.
    if (from->is_empty())
        workspace_manager.delete_workspace(from->id());
    else
        from->hide(from_end);
    return true;
}

void Output::advise_application_zone_create(miral::Zone const& application_zone)
{
    if (application_zone.extents().contains(area))
    {
        application_zone_list.push_back(application_zone);
        for (auto& workspace : workspaces)
            workspace->recalculate_area();
    }
}

void Output::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto& zone : application_zone_list)
        if (zone == original)
        {
            zone = updated;
            for (auto const& workspace : workspaces)
                workspace->recalculate_area();
            break;
        }
}

void Output::advise_application_zone_delete(miral::Zone const& application_zone)
{
    auto const original_size = application_zone_list.size();
    application_zone_list.erase(
        std::remove(application_zone_list.begin(), application_zone_list.end(), application_zone),
        application_zone_list.end());

    if (application_zone_list.size() != original_size)
    {
        for (auto const& workspace : workspaces)
            workspace->recalculate_area();
    }
}

bool Output::point_is_in_output(int x, int y)
{
    return area.contains(geom::Point(x, y));
}

void Output::update_area(geom::Rectangle const& new_area)
{
    area = new_area;
    for (auto& workspace : workspaces)
        workspace->set_area(area);
}

void Output::graft(std::shared_ptr<Container> const& container)
{
    active()->graft(container);
}

[[nodiscard]] WorkspaceInterface const* Output::workspace(uint32_t id) const
{
    for (auto const& workspace : workspaces)
    {
        if (workspace->id() == id)
            return workspace.get();
    }

    return nullptr;
}

glm::mat4 Output::get_transform() const
{
    return transform;
}

void Output::set_transform(glm::mat4 const& in)
{
    transform = in;
}

void Output::set_info(int next_id, std::string next_name)
{
    id_ = next_id;
    name_ = std::move(next_name);
}

void Output::set_defunct()
{
    is_defunct_ = true;
}

void Output::unset_defunct()
{
    is_defunct_ = false;
}

nlohmann::json Output::to_json(bool is_focused) const
{
    auto active = output_config.used;
    nlohmann::json nodes = nlohmann::json::array();
    for (auto const& workspace : workspaces)
    {
        if (workspace)
            nodes.push_back(workspace->to_json(is_focused));
    }

    nlohmann::json modes_node;
    for (auto const& mode : output_config.modes)
    {
        nlohmann::json mode_node;
        mode_node["width"] = mode.size.width.as_int();
        mode_node["height"] = mode.size.height.as_int();
        mode_node["refresh"] = mode.vrefresh_hz * 1000;
        modes_node.push_back(mode_node);
    }

    auto const current_mode = output_config.current_mode_index && output_config.current_mode_index < output_config.modes.size()
        ? output_config.modes[*output_config.current_mode_index]
        : mir::graphics::DisplayConfigurationMode(mir::geometry::Size(0, 0), 0);
    nlohmann::json current_mode_node;
    current_mode_node["width"] = current_mode.size.width.as_int();
    current_mode_node["height"] = current_mode.size.height.as_int();
    current_mode_node["refresh"] = current_mode.vrefresh_hz * 1000;

    nlohmann::json transform;
    switch (output_config.orientation)
    {
    case mir_orientation_normal:
        transform = "normal";
        break;
    case mir_orientation_left:
        transform = "90";
        break;
    case mir_orientation_inverted:
        transform = "180";
        break;
    case mir_orientation_right:
        transform = "270";
        break;
    }

    auto const make = output_config.display_info.vendor.value_or("Unknown");
    auto const model = output_config.display_info.model.value_or("Unknown");
    auto const serial = output_config.display_info.serial.value_or("0x00000000");

    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this)        },
        { "name",                 name_                                         },
        { "active",               active                                        },
        { "dpms",                 output_config.power_mode == mir_power_mode_on },
        { "scale",                output_config.scale                           },
        { "scale_filter",         "linear"                                      }, // Deprecated
        { "adaptive_sync_status", false                                         }, /// TODO: Supply this value
        { "make",                 make                                          },
        { "model",                model                                         },
        { "serial",               serial                                        },
        { "transform",            transform                                     },
        { "type",                 "output"                                      },
        { "layout",               "output"                                      },
        { "orientation",          "none"                                        },
        { "visible",              true                                          },
        { "focused",              is_focused                                    },
        { "urgent",               false                                         },
        { "border",               "none"                                        },
        { "current_border_width", 0                                             },
        { "window_rect",          {
                             { "x", 0 },
                             { "y", 0 },
                             { "width", 0 },
                             { "height", 0 },
                         }                              },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", 0 },
                           { "height", 0 },
                       }                                  },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", 0 },
                          { "height", 0 },
                      }                                    },
        { "rect",                 {
                      { "x", area.top_left.x.as_int() },
                      { "y", area.top_left.y.as_int() },
                      { "width", area.size.width.as_int() },
                      { "height", area.size.height.as_int() },
                  }                                            },
        { "nodes",                nodes                                         },
        { "modes",                modes_node                                    },
        { "current_mode",         current_mode_node                             }
    };
}

nlohmann::json Output::get_outputs_json(bool) const
{
    auto active_workspace = active();
    nlohmann::json workspace;

    if (active_workspace)
        workspace = { "current_workspace", active_workspace->display_name() };
    else
        workspace = { "current_workspace", nullptr };

    auto active = output_config.used;

    nlohmann::json modes_node;
    for (auto const& mode : output_config.modes)
    {
        nlohmann::json mode_node;
        mode_node["width"] = mode.size.width.as_int();
        mode_node["height"] = mode.size.height.as_int();
        mode_node["refresh"] = mode.vrefresh_hz * 1000;
        modes_node.push_back(mode_node);
    }

    auto const current_mode = output_config.current_mode_index && output_config.current_mode_index < output_config.modes.size()
        ? output_config.modes[*output_config.current_mode_index]
        : mir::graphics::DisplayConfigurationMode(mir::geometry::Size(0, 0), 0);
    nlohmann::json current_mode_node;
    current_mode_node["width"] = current_mode.size.width.as_int();
    current_mode_node["height"] = current_mode.size.height.as_int();
    current_mode_node["refresh"] = current_mode.vrefresh_hz * 1000;

    auto const primary = is_primary();

    std::string subpixel_hinting;
    switch (output_config.current_format)
    {
    case mir_pixel_format_abgr_8888:
    case mir_pixel_format_xbgr_8888:
    case mir_pixel_format_bgr_888:
        subpixel_hinting = "bgr";
        break;
    case mir_pixel_format_argb_8888:
    case mir_pixel_format_xrgb_8888:
    case mir_pixel_format_rgb_888:
    case mir_pixel_format_rgb_565:
    case mir_pixel_format_rgba_5551:
    case mir_pixel_format_rgba_4444:
    default:
        subpixel_hinting = "rgb";
        break;
    }

    auto const make = output_config.display_info.vendor.value_or("Unknown");
    auto const model = output_config.display_info.model.value_or("Unknown");
    auto const serial = output_config.display_info.serial.value_or("0x00000000");

    return {
        { "name",             name_                                         },
        { "make",             make                                          },
        { "model",            model                                         },
        { "serial",           serial                                        },
        { "active",           active                                        },
        { "dpms",             output_config.power_mode == mir_power_mode_on },
        { "power",            output_config.power_mode == mir_power_mode_on },
        { "primary",          primary                                       },
        { "scale",            active ? output_config.scale : -1             },
        { "subpixel_hinting", subpixel_hinting                              },
        workspace,
        { "rect",             {
                      { "x", area.top_left.x.as_int() },
                      { "y", area.top_left.y.as_int() },
                      { "width", area.size.width.as_int() },
                      { "height", area.size.height.as_int() },
                  }                                        },
        { "modes",            modes_node                                    },
        { "current_mode",     current_mode_node                             },
    };
}

bool Output::is_primary() const
{
    return output_config.is_primary;
}
