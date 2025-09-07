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

#define MIR_LOG_COMPONENT "output_content"
#define GLM_ENABLE_EXPERIMENTAL

#include "output.h"
#include "animator.h"
#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "mir_version_manager.h"
#include "policy.h"
#include "vector_helpers.h"
#include "workspace.h"
#include "workspace_manager.h"
#include <glm/gtx/transform.hpp>
#include <memory>
#include <mir/log.h>
#include <miral/window_info.h>
#include <miral/zone.h>
#ifndef MIR_VERSION_2_22_OR_GREATER
#include <mir/graphics/edid.h>
#endif

using namespace miracle;
namespace mg = mir::graphics;

Output::Output(
    Policy* policy,
    std::string name,
    int id,
    geom::Rectangle const& area,
    OutputConfigDetails const& output_config,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Animator> const& animator) :
    policy { policy },
    name_ { std::move(name) },
    id_ { id },
    area { area },
    output_config { output_config },
    state { state },
    config { config },
    window_controller { window_controller },
    animator { animator },
    handle { animator->register_animateable() },
    position_offset { 0.f, 0.f },
    transform { glm::mat4(1.f) },
    final_transform { glm::mat4(1.f) },
    is_defunct_ { false }
{
}

Output::~Output()
{
    animator->remove_by_animation_handle(handle);
}

std::shared_ptr<WorkspaceInterface> Output::active() const
{
    if (active_workspace.expired())
        return nullptr;

    return active_workspace.lock();
}

std::shared_ptr<Container> Output::intersect(float x, float y)
{
    // If the output is animating, then we can't trust any pointer events.
    if (animator->is_animating(handle))
        return nullptr;

    if (auto const window = window_controller->window_at(x, y))
        return window_controller->get_container(window);

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
        shared_from_this(), data.id, data.num, data.name, config, window_controller, state, data.registrar);
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
        to->show();

        auto const to_rectangle = get_workspace_rectangle(to_index);
        set_position(glm::vec2(
            -to_rectangle.top_left.x.as_int(),
            -to_rectangle.top_left.y.as_int()));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        to->workspace_transform_change_hack();
#pragma GCC diagnostic pop
        return true;
    }

    // If we have a 'from' and a 'to', we want to animate between them.

    // Note: It is very important that [active_workspace] be modified before notifications
    // are sent out.
    active_workspace = to;

    auto const from_src = get_workspace_rectangle(from_index);
    auto const to_src = get_workspace_rectangle(to_index);
    from->transfer_pinned_windows_to(to);

    geom::Rectangle const real {
        { geom::X { position_offset.x }, geom::Y { position_offset.y } },
        area.size
    };
    geom::Rectangle const src {
        { geom::X { -from_src.top_left.x.as_int() }, geom::Y { from_src.top_left.y.as_int() } },
        area.size
    };
    geom::Rectangle const dest {
        { geom::X { -to_src.top_left.x.as_int() }, geom::Y { to_src.top_left.y.as_int() } },
        area.size
    };

    // If 'from' is empty, we can delete the workspace. However, this means that from_src is now incorrect
    if (from->is_empty())
        workspace_manager.delete_workspace(from->id());

    if (!config->are_animations_enabled())
    {
        to->show();
        from->hide();
        to->on_animation_start();
        handle_workspace_animation(
            AnimationFrameResult {
                true,
                dest,
                glm::mat4(1.f),
                std::nullopt },
            to,
            from);
        to->on_animation_end();
        return true;
    }

    auto const animation = std::make_shared<WorkspaceAnimation>(
        handle,
        config->get_animation_definition(AnimateableEvent::workspace_switch),
        src,
        dest,
        real,
        to,
        from,
        this);

    animator->append(animation);

    // Show all workspaces so that we can animate over all workspaces.
    // Important: Make sure that we show _after_ the "append" has
    // happened so that we are showing with the correct initial
    // transform.
    for (auto const& workspace : workspaces)
    {
        if (workspace != from)
            workspace->show();
    }

    to->on_animation_start();
    return true;
}

Output::WorkspaceAnimation::WorkspaceAnimation(
    AnimationHandle handle,
    AnimationDefinition definition,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current,
    std::shared_ptr<WorkspaceInterface> const& to_workspace,
    std::shared_ptr<WorkspaceInterface> const& from_workspace,
    Output* output) :
    MultiBuiltInAnimation(handle, definition, from, to, current),
    to_workspace { to_workspace },
    from_workspace { from_workspace },
    output { output }
{
}

void Output::WorkspaceAnimation::on_tick(miracle::AnimationFrameResult const& asr)
{
    output->policy->main_loop()->enqueue(this, [asr = asr, to_workspace = to_workspace, from_workspace = from_workspace, output = output]()
    {
        output->policy->handle_workspace_animation(asr, to_workspace, from_workspace);
    });
}

void Output::handle_workspace_animation(
    AnimationFrameResult const& asr,
    std::shared_ptr<WorkspaceInterface> const& to,
    std::shared_ptr<WorkspaceInterface> const&)
{
    auto const rectangle = asr.rectangle;
    if (asr.is_complete)
    {
        if (rectangle)
            set_position(glm::vec2(rectangle->top_left.x.as_int(), rectangle->top_left.y.as_int()));
        if (asr.transform)
            set_transform(asr.transform.value());

        for (auto const& workspace : workspaces)
        {
            if (workspace != to)
                workspace->hide();
        }

        to->workspace_transform_change_hack();
        to->on_animation_end();
        return;
    }

    if (rectangle)
        set_position(glm::vec2(rectangle->top_left.x.as_int(), rectangle->top_left.y.as_int()));
    if (asr.transform)
        set_transform(asr.transform.value());

    for (auto const& workspace : workspaces)
    {
        if (!workspace)
            continue;

        workspace->workspace_transform_change_hack();
    }
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

geom::Rectangle Output::get_workspace_rectangle(size_t i) const
{
    // TODO: Support vertical workspaces one day in the future
    auto const& workspace = workspaces[i];
    size_t x = 0;
    if (workspace->num())
        x = static_cast<size_t>((workspace->num().value() - 1) * area.size.width.as_int());
    else
    {
        // Find the index of the first non-numbered workspace
        size_t j = 0;
        for (; j < workspaces.size(); j++)
        {
            if (workspaces[j]->num() == std::nullopt)
                break;
        }

        x = (WorkspaceManager::NUM_DEFAULT_WORKSPACES - 1) + (i - j) * static_cast<size_t>(area.size.width.as_int());
    }

    return geom::Rectangle {
        geom::Point { geom::X { x },            geom::Y { 0 }             },
        geom::Size { area.size.width.as_int(), area.size.height.as_int() }
    };
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
    return final_transform;
}

void Output::set_transform(glm::mat4 const& in)
{
    transform = in;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

void Output::set_position(glm::vec2 const& v)
{
    position_offset = v;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
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

    auto const& current_mode = output_config.modes[output_config.current_mode_index];
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

#ifdef MIR_VERSION_2_22_OR_GREATER
    auto const make = output_config.display_info.vendor.value_or("Unknown");
    auto const model = output_config.display_info.model.value_or("Unknown");
    auto const serial = output_config.display_info.serial.value_or("0x00000000");
#else
    std::string make = "Unknown";
    std::string model = "Unknown";
    std::string serial = "0x00000000";
    if (output_config.edid.size() >= mg::Edid::minimum_size)
    {
        auto const edid = reinterpret_cast<mg::Edid const*>(output_config.edid.data());
        mg::Edid::Manufacturer man;
        edid->get_manufacturer(man);
        mg::Edid::MonitorName name;
        edid->get_monitor_name(name);

        make = man;
        model = name;
#ifdef MIR_VERSION_2_21_OR_GREATER
        serial = edid->serial_number();
#endif
    }
#endif

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

    auto const& current_mode = output_config.modes[output_config.current_mode_index];
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

#ifdef MIR_VERSION_2_22_OR_GREATER
    auto const make = output_config.display_info.vendor.value_or("Unknown");
    auto const model = output_config.display_info.model.value_or("Unknown");
    auto const serial = output_config.display_info.serial.value_or("0x00000000");
#else
    std::string make = "Unknown";
    std::string model = "Unknown";
    std::string serial = "0x00000000";
    if (output_config.edid.size() >= mg::Edid::minimum_size)
    {
        auto const edid = reinterpret_cast<mg::Edid const*>(output_config.edid.data());
        mg::Edid::Manufacturer man;
        edid->get_manufacturer(man);
        mg::Edid::MonitorName name;
        edid->get_monitor_name(name);

        make = man;
        model = name;
#ifdef MIR_VERSION_2_21_OR_GREATER
        serial = edid->serial_number();
#endif
    }
#endif

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
