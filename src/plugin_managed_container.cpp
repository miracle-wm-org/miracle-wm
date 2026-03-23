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

#include "plugin_managed_container.h"
#include "abstract_output.h"
#include "abstract_workspace.h"
#include "compositor_state.h"
#include "container_listener.h"
#include "window_controller.h"
#include <mir/scene/surface.h>

using namespace miracle;

namespace
{
inline glm::mat4 workspace_transform(Container const& container)
{
    return container.get_output_transform() * container.get_workspace_transform();
}
}

void PluginManagedContainer::show()
{
    auto show_state = cached.value_or(mir_window_state_restored);
    if (show_state == mir_window_state_hidden)
        show_state = mir_window_state_restored;
    window_controller->change_state(window_, show_state);
}

void PluginManagedContainer::hide()
{
    cached = window_controller->info_for(window_).state();
    window_controller->change_state(window_, mir_window_state_hidden);
}

geom::Rectangle PluginManagedContainer::get_logical_area() const
{
    return geom::Rectangle(window_.top_left(), window_.size());
}

void PluginManagedContainer::set_logical_area(
    geom::Rectangle const& area, bool with_animations)
{
    window_controller->set_rectangle(window_, get_visible_area(), area, with_animations);
}

PluginManagedContainer::PluginManagedContainer(
    PluginHandle plugin_handle,
    miral::Window const& window,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<AbstractWorkspace> const& workspace,
    glm::mat4 transform,
    float alpha,
    bool resizable,
    bool movable) :
    WindowContainer(compositor_state->next_container_id(), compositor_state->render_data_manager(), window_controller),
    plugin_handle_ { plugin_handle },
    cached { window_controller->info_for(window).state() },
    window_controller { window_controller },
    compositor_state { compositor_state },
    workspace_ { workspace }
{
    resizable_ = resizable;
    movable_ = movable;
    associate_to_window(window);
    set_window_alpha(alpha);
    set_window_transform(transform);
}

geom::Rectangle PluginManagedContainer::get_visible_area() const
{
    return get_logical_area();
}

void PluginManagedContainer::constrain()
{
    window_controller->noclip(window_);
}

std::weak_ptr<ParentContainer> PluginManagedContainer::get_parent() const
{
    return std::weak_ptr<ParentContainer>();
}

void PluginManagedContainer::commit_changes()
{
}

void PluginManagedContainer::set_parent(std::shared_ptr<ParentContainer> const&)
{
    // noop, plugin containers have no parent
}

size_t PluginManagedContainer::get_min_height() const
{
    return 0;
}

size_t PluginManagedContainer::get_min_width() const
{
    return 0;
}

void PluginManagedContainer::handle_ready()
{
    window_controller->select_active_window(window_);
}

void PluginManagedContainer::handle_modify(miral::WindowSpecification const& specification)
{
    window_controller->modify(window_, specification);
}

void PluginManagedContainer::handle_request_move(MirInputEvent const*)
{
    if (!movable_)
        return;
}

void PluginManagedContainer::handle_raise()
{
    window_controller->select_active_window(window_);
}

bool PluginManagedContainer::resize(Direction, int)
{
    if (!resizable_)
        return false;
    return false;
}

bool PluginManagedContainer::set_size(std::optional<int> const&, std::optional<int> const&)
{
    if (!resizable_)
        return false;
    return false;
}

bool PluginManagedContainer::toggle_fullscreen()
{
    return false;
}

void PluginManagedContainer::request_horizontal_layout()
{
}

void PluginManagedContainer::request_vertical_layout()
{
}

void PluginManagedContainer::toggle_layout(bool)
{
}

void PluginManagedContainer::on_move_to(geom::Point const&)
{
}

void PluginManagedContainer::on_resize(geom::Size const&)
{
}

mir::geometry::Rectangle PluginManagedContainer::confirm_placement(
    MirWindowState, mir::geometry::Rectangle const& rectangle)
{
    return rectangle;
}

std::shared_ptr<AbstractWorkspace> PluginManagedContainer::get_workspace() const
{
    return workspace_.lock();
}

void PluginManagedContainer::set_workspace(std::shared_ptr<AbstractWorkspace> const& workspace)
{
    workspace_ = workspace;
    for_each_observer([this](ContainerListener* observer)
    {
        observer->on_container_workspace_changed(*this);
    });
}

std::shared_ptr<AbstractOutput> PluginManagedContainer::get_output() const
{
    if (auto const workspace = workspace_.lock())
        return workspace->get_output();
    return nullptr;
}

glm::mat4 PluginManagedContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

uint32_t PluginManagedContainer::animation_handle() const
{
    return handle_;
}

void PluginManagedContainer::animation_handle(uint32_t handle)
{
    handle_ = handle;
}

bool PluginManagedContainer::is_focused() const
{
    return is_focused_;
}

bool PluginManagedContainer::is_fullscreen() const
{
    return false;
}

bool PluginManagedContainer::select_next(Direction)
{
    return false;
}

bool PluginManagedContainer::pinned() const
{
    return false;
}

bool PluginManagedContainer::pinned(bool)
{
    return false;
}

bool PluginManagedContainer::move(Direction)
{
    return false;
}

bool PluginManagedContainer::move_by(Direction, int)
{
    return false;
}

bool PluginManagedContainer::move_to(Container&)
{
    return false;
}

bool PluginManagedContainer::move_to(int x, int y, bool)
{
    if (!movable_)
        return false;
    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    window_controller->modify(window_, spec);
    constrain();
    return true;
}

bool PluginManagedContainer::move_by(float, float)
{
    if (!movable_)
        return false;
    return false;
}

bool PluginManagedContainer::toggle_tabbing()
{
    return false;
}

bool PluginManagedContainer::toggle_stacking()
{
    return false;
}

bool PluginManagedContainer::drag_start()
{
    return false;
}

void PluginManagedContainer::drag(int, int)
{
}

bool PluginManagedContainer::drag_stop()
{
    return false;
}

bool PluginManagedContainer::set_layout(LayoutScheme)
{
    return false;
}

bool PluginManagedContainer::anchored() const
{
    return false;
}

void PluginManagedContainer::scratchpad_state(ScratchpadState)
{
}

ScratchpadState PluginManagedContainer::scratchpad_state() const
{
    return ScratchpadState::none;
}

std::vector<std::string> const& PluginManagedContainer::get_marks() const
{
    return Container::get_marks();
}

LayoutScheme PluginManagedContainer::get_layout() const
{
    return LayoutScheme::none;
}

bool PluginManagedContainer::matches(ContainerScope const&) const
{
    return false;
}

nlohmann::json PluginManagedContainer::to_json(bool) const
{
    auto const app = window_.application();
    auto const& win_info = window_controller->info_for(window_);
    auto const visible_area = get_visible_area();
    auto const logical_area = get_logical_area();
    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this) },
        { "name",                 app->name()                            },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                     },
        { "focused",              is_focused()                           },
        { "focus",                std::vector<int>()                     },
        { "border",               "none"                                 },
        { "current_border_width", 0                                      },
        { "layout",               "none"                                 },
        { "orientation",          "none"                                 },
        { "window_rect",          {
                             { "x", visible_area.top_left.x.as_int() },
                             { "y", visible_area.top_left.y.as_int() },
                             { "width", visible_area.size.width.as_int() },
                             { "height", visible_area.size.height.as_int() },
                         }                       },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", logical_area.size.width.as_int() },
                           { "height", logical_area.size.height.as_int() },
                       }                           },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                             },
        { "window",               0                                      },
        { "urgent",               false                                  },
        { "floating_nodes",       std::vector<int>()                     },
        { "sticky",               false                                  },
        { "type",                 "plugin"                               },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                },
        { "pid",                  app->process_id()                      },
        { "app_id",               win_info.application_id()              },
        { "visible",              true                                   },
        { "shell",                "miracle-wm"                           },
        { "inhibit_idle",         false                                  },
        { "idle_inhibitors",      {
                                 { "application", "none" },
                                 { "user", "visible" },
                             }               },
        { "window_properties",    nlohmann::json::object()               },
        { "nodes",                std::vector<int>()                     },
    };
}

std::optional<PluginHandle> PluginManagedContainer::plugin_handle() const
{
    return plugin_handle_;
}
