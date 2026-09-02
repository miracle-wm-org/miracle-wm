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

#include "freestyle_window_container.h"
#include "abstract_output.h"
#include "abstract_workspace.h"
#include "compositor_state.h"
#include "config.h"
#include "container_listener.h"
#include "direction.h"
#include "window_controller.h"
#include "window_helpers.h"
#include <mir/scene/surface.h>

using namespace miracle;

FreestyleWindowContainer::FreestyleWindowContainer(
    miral::Window const& window,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<AbstractWorkspace> const& workspace,
    std::shared_ptr<Config> const& config,
    bool has_border) :
    WindowContainer(compositor_state->next_container_id(), compositor_state->render_data_manager(), window_controller),
    window_controller { window_controller },
    compositor_state { compositor_state },
    config { config },
    has_border_ { has_border },
    workspace_ { workspace }
{
    associate_to_window(window);
}

void FreestyleWindowContainer::show()
{
    auto const w = window_;
    auto restore = restore_result_;
    restore_result_.reset();
    if (restore)
        window_controller->show(w, restore.value());
}

void FreestyleWindowContainer::hide()
{
    auto const w = window_;
    restore_result_ = window_controller->hide(w);
}

geom::Rectangle FreestyleWindowContainer::get_logical_area() const
{
    auto const w = window_;
    return geom::Rectangle(w.top_left(), w.size());
}

void FreestyleWindowContainer::set_logical_area(geom::Rectangle const& area, bool with_animations)
{
    window_controller->set_rectangle(window_, get_visible_area(), area, with_animations);
}

geom::Rectangle FreestyleWindowContainer::get_visible_area() const
{
    return get_logical_area();
}

void FreestyleWindowContainer::constrain()
{
    auto const w = window_;
    window_controller->noclip(w);
}

std::weak_ptr<ParentContainer> FreestyleWindowContainer::get_parent() const
{
    return std::weak_ptr<ParentContainer>();
}

void FreestyleWindowContainer::commit_changes()
{
    auto const w = window_;
    auto const render_id = render_id_;

    if (!next_state_)
        return;

    bool const entering_fullscreen = next_state_ == mir_window_state_fullscreen;
    bool const leaving_fullscreen = window_controller->get_state(w) == mir_window_state_fullscreen;

    if (entering_fullscreen || leaving_fullscreen)
    {
        for_each_observer([this](ContainerListener* observer)
        {
            observer->on_container_fullscreen(*this);
        });
    }

    window_controller->change_state(w, next_state_.value());

    if (entering_fullscreen || leaving_fullscreen)
        update_window_margins(has_border_ ? config->get_border_config().size : 0, entering_fullscreen);

    if (render_id.has_value())
        if (auto const r = rdm.lock())
            r->needs_outline_change(render_id.value(), !entering_fullscreen);

    next_state_.reset();

    if (next_depth_layer_)
    {
        miral::WindowSpecification spec;
        spec.depth_layer() = next_depth_layer_.value();
        window_controller->modify(w, spec);
        next_depth_layer_.reset();
    }

    auto const saved_area = pre_fullscreen_area_;
    bool const restoring = !entering_fullscreen;

    if (restoring && saved_area)
    {
        window_controller->set_rectangle(w, get_visible_area(), saved_area.value(), true);
        pre_fullscreen_area_.reset();
    }

    constrain();
}

void FreestyleWindowContainer::set_parent(std::shared_ptr<ParentContainer> const&)
{
}

size_t FreestyleWindowContainer::get_min_height() const
{
    auto const& info = window_controller->info_for(window_);
    if (info.min_height().as_value() == 0)
        return 50;
    return info.min_height().as_value();
}

size_t FreestyleWindowContainer::get_min_width() const
{
    auto const& info = window_controller->info_for(window_);
    if (info.min_width().as_value() == 0)
        return 50;
    return info.min_width().as_value();
}

void FreestyleWindowContainer::handle_ready()
{
    if (has_border_)
    {
        int const border_size = config->get_border_config().size;
        auto const w = window_;
        auto surface = w.operator std::shared_ptr<mir::scene::Surface>();
        surface->set_window_margins(
            mir::geometry::DeltaY { border_size },
            mir::geometry::DeltaX { border_size },
            mir::geometry::DeltaY { border_size },
            mir::geometry::DeltaX { border_size });
    }
    window_controller->select_active_window(window_);
}

void FreestyleWindowContainer::handle_modify(miral::WindowSpecification const& specification, bool hidden)
{
    auto const w = window_;
    auto mods = specification;
    if (mods.state())
    {
        if (hidden)
        {
            // This container's workspace is not being rendered. Defer the state change
            // until the container is shown again (via restore_result) so the workspace
            // stays hidden; non-state modifications below are still applied immediately.
            if (restore_result_)
                restore_result_->state = mods.state().value();
            else
                restore_result_ = RestoreResult { mods.state().value() };
            window_helpers::reset_optional(mods.state());
        }
        else
        {
            window_controller->change_state(w, mods.state().value());
        }
    }
    window_controller->modify(w, mods);
}

void FreestyleWindowContainer::handle_request_move(MirInputEvent const*)
{
}

void FreestyleWindowContainer::handle_raise()
{
    window_controller->select_active_window(window_);
}

void FreestyleWindowContainer::on_focus_gained()
{
    WindowContainer::on_focus_gained();
    window_controller->raise(window_);
}

bool FreestyleWindowContainer::resize(Direction direction, int pixels)
{
    if (is_maximized())
    {
        restore_from_maximize();
        return true;
    }

    auto const area = get_logical_area();
    int new_x = area.top_left.x.as_int();
    int new_y = area.top_left.y.as_int();
    int new_w = area.size.width.as_int();
    int new_h = area.size.height.as_int();

    switch (direction)
    {
    case Direction::right:
        new_w += pixels;
        break;
    case Direction::left:
        new_w -= pixels;
        break;
    case Direction::down:
        new_h += pixels;
        break;
    case Direction::up:
        new_h -= pixels;
        break;
    default:
        return false;
    }

    set_logical_area(geom::Rectangle(geom::Point(new_x, new_y), geom::Size(new_w, new_h)), true);
    return true;
}

bool FreestyleWindowContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    if (is_maximized())
    {
        restore_from_maximize();
        return true;
    }

    auto area = get_logical_area();
    area.size = {
        width.value_or(area.size.width.as_int()),
        height.value_or(area.size.height.as_int())
    };
    set_logical_area(area, true);
    return true;
}

bool FreestyleWindowContainer::toggle_fullscreen()
{
    {
        if (is_fullscreen())
        {
            next_state_ = mir_window_state_restored;
            next_depth_layer_ = mir_depth_layer_always_on_top;
        }
        else
        {
            pre_fullscreen_area_ = get_logical_area();
            next_state_ = mir_window_state_fullscreen;
            next_depth_layer_ = mir_depth_layer_above;
        }
    }
    commit_changes();
    return true;
}

void FreestyleWindowContainer::request_horizontal_layout()
{
}

void FreestyleWindowContainer::request_vertical_layout()
{
}

void FreestyleWindowContainer::toggle_layout(bool)
{
}

void FreestyleWindowContainer::on_move_to(geom::Point const&)
{
}

void FreestyleWindowContainer::on_resize(geom::Size const&)
{
}

mir::geometry::Rectangle FreestyleWindowContainer::confirm_placement(
    MirWindowState, mir::geometry::Rectangle const& rectangle)
{
    return rectangle;
}

std::shared_ptr<AbstractWorkspace> FreestyleWindowContainer::get_workspace() const
{
    return workspace_.lock();
}

void FreestyleWindowContainer::set_workspace(std::shared_ptr<AbstractWorkspace> const& workspace)
{
    workspace_ = workspace;
    for_each_observer([this](ContainerListener* observer)
    {
        observer->on_container_workspace_changed(*this);
    });
}

std::shared_ptr<AbstractOutput> FreestyleWindowContainer::get_output() const
{
    if (auto const workspace = workspace_.lock())
        return workspace->get_output();
    return nullptr;
}

glm::mat4 FreestyleWindowContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

uint32_t FreestyleWindowContainer::animation_handle() const
{
    return handle_;
}

void FreestyleWindowContainer::animation_handle(uint32_t handle)
{
    handle_ = handle;
}

bool FreestyleWindowContainer::is_focused() const
{
    return is_focused_;
}

bool FreestyleWindowContainer::is_fullscreen() const
{
    return window_controller->get_state(window_) == mir_window_state_fullscreen;
}

bool FreestyleWindowContainer::is_maximized() const
{
    return window_controller->get_state(window_) == mir_window_state_maximized;
}

void FreestyleWindowContainer::restore_from_maximize()
{
    window_controller->change_state(window_, mir_window_state_restored);
}

bool FreestyleWindowContainer::needs_outline() const
{
    return has_border_;
}

bool FreestyleWindowContainer::select_next(Direction)
{
    return false;
}

bool FreestyleWindowContainer::pinned() const
{
    return pinned_;
}

bool FreestyleWindowContainer::pinned(bool next)
{
    pinned_ = next;
    return true;
}

bool FreestyleWindowContainer::move(Direction)
{
    return false;
}

bool FreestyleWindowContainer::move_by(Direction direction, int pixels)
{
    auto const area = get_logical_area();
    int x = area.top_left.x.as_int();
    int y = area.top_left.y.as_int();

    if (is_vertical_direction(direction))
        y += is_negative_direction(direction) ? -pixels : pixels;
    else
        x += is_negative_direction(direction) ? -pixels : pixels;

    return move_to(x, y, true);
}

bool FreestyleWindowContainer::move_to(Container&)
{
    return false;
}

bool FreestyleWindowContainer::move_to(int x, int y, bool)
{
    if (is_maximized())
    {
        restore_from_maximize();
        return true;
    }

    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    window_controller->modify(window_, spec);
    constrain();
    return true;
}

bool FreestyleWindowContainer::move_by(float, float)
{
    return false;
}

bool FreestyleWindowContainer::toggle_tabbing()
{
    return false;
}

bool FreestyleWindowContainer::toggle_stacking()
{
    return false;
}

bool FreestyleWindowContainer::drag_start()
{
    if (is_dragging_)
        return false;
    if (is_maximized())
        restore_from_maximize();
    is_dragging_ = true;
    constrain();
    return true;
}

void FreestyleWindowContainer::drag(int x, int y)
{
    if (!is_dragging_)
        return;
    dragged_position_ = { x, y };
    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    window_controller->modify(window_, spec);
}

bool FreestyleWindowContainer::drag_stop()
{
    if (!is_dragging_)
        return false;
    is_dragging_ = false;
    for_each_observer([this](ContainerListener* observer)
    {
        observer->on_container_moved(*this);
    });
    constrain();
    return true;
}

bool FreestyleWindowContainer::set_layout(LayoutScheme)
{
    return false;
}

bool FreestyleWindowContainer::anchored() const
{
    return false;
}

void FreestyleWindowContainer::scratchpad_state(ScratchpadState)
{
}

ScratchpadState FreestyleWindowContainer::scratchpad_state() const
{
    return ScratchpadState::none;
}

std::vector<std::string> const& FreestyleWindowContainer::get_marks() const
{
    return Container::get_marks();
}

LayoutScheme FreestyleWindowContainer::get_layout() const
{
    return LayoutScheme::none;
}

bool FreestyleWindowContainer::matches(ContainerScope const&) const
{
    return false;
}

bool FreestyleWindowContainer::can_animate()
{
    // Transient surfaces (popups, tooltips, dialogs, satellites) must not animate.
    // Native wayland popups arrive typed menu/tip, but xwayland popups arrive as
    // parented normal/freestyle windows, so the parent check is load-bearing.
    auto const& info = window_controller->info_for(window_);
    if (info.parent())
        return false;
    return info.type() == mir_window_type_freestyle || info.type() == mir_window_type_normal;
}

nlohmann::json FreestyleWindowContainer::to_json(bool) const
{
    auto const w = window_;
    auto const app = w.application();
    auto const& win_info = window_controller->info_for(w);
    auto const visible_area = get_visible_area();
    auto const logical_area = get_logical_area();
    int const border_width = has_border_ ? config->get_border_config().size : 0;
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
        { "border",               has_border_ ? "normal" : "none"        },
        { "current_border_width", border_width                           },
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
        { "urgent",               urgent()                               },
        { "floating_nodes",       std::vector<int>()                     },
        { "sticky",               false                                  },
        { "type",                 "floating_con"                         },
        { "fullscreen_mode",      0                                      },
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
