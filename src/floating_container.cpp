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

#define MIR_LOG_COMPONENT "floating_container"

#include "floating_container.h"
#include "leaf_container.h"
#include "workspace.h"
#include "output.h"
#include "compositor_state.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <mir/log.h>

using namespace miracle;

FloatingContainer::FloatingContainer(
    miral::Window const& window,
    miral::MinimalWindowManager& wm,
    WindowController& window_controller,
    Workspace* workspace)
    : window_{window},
      wm{wm},
      window_controller{window_controller},
      workspace_{workspace}
{
}

void FloatingContainer::commit_changes()
{

}

mir::geometry::Rectangle FloatingContainer::get_logical_area() const
{
    return get_visible_area();
}

void FloatingContainer::set_logical_area(mir::geometry::Rectangle const& rectangle)
{
    window_controller.set_rectangle(window_, get_visible_area(), rectangle);
}

mir::geometry::Rectangle FloatingContainer::get_visible_area() const
{
    return {
        window_.top_left(),
        window_.size()
    };
}

void FloatingContainer::constrain() {}

void FloatingContainer::set_parent(std::shared_ptr<ParentContainer> const& parent)
{
    throw std::logic_error("FloatingContainer cannot have a parent");
}

size_t FloatingContainer::get_min_height() const
{
    return 1;
}

size_t FloatingContainer::get_min_width() const
{
    return 1;
}

void FloatingContainer::handle_ready()
{
    auto& info = window_controller.info_for(window_);
    wm.handle_window_ready(info);
}

void FloatingContainer::handle_modify(miral::WindowSpecification const& modifications)
{
    auto& info = window_controller.info_for(window_);
    wm.handle_modify_window(info, modifications);
}

void FloatingContainer::handle_request_move(MirInputEvent const* input_event)
{
    wm.handle_request_move(
        window_controller.info_for(window_), input_event);
}

void FloatingContainer::handle_request_resize(
    MirInputEvent const* input_event, MirResizeEdge edge)
{
    wm.handle_request_resize(
        window_controller.info_for(window_), input_event, edge);
}

void FloatingContainer::handle_raise()
{
    wm.handle_raise_window(window_controller.info_for(window_));
}

void FloatingContainer::on_open()
{
    window_controller.open(window_);
}

void FloatingContainer::on_focus_gained()
{
    wm.advise_focus_gained(window_controller.info_for(window_));
}

void FloatingContainer::on_focus_lost()
{
    wm.advise_focus_lost(window_controller.info_for(window_));
}

void FloatingContainer::on_move_to(geom::Point const& top_left)
{
    wm.advise_move_to(window_controller.info_for(window_), top_left);
}

mir::geometry::Rectangle FloatingContainer::confirm_placement(
    MirWindowState state, mir::geometry::Rectangle const& placement)
{
    return wm.confirm_placement_on_display(
        window_controller.info_for(window_),
        state,
        placement
    );
}

bool FloatingContainer::pinned() const
{
    return is_pinned;
}

bool FloatingContainer::pinned(bool in)
{
    is_pinned = in;
    return true;
}

std::optional<miral::Window> FloatingContainer::window() const
{
    return window_;
}

bool FloatingContainer::resize(Direction direction)
{
    return false;
}

bool FloatingContainer::toggle_fullscreen()
{
    return false;
}

void FloatingContainer::request_horizontal_layout()
{
}

void FloatingContainer::request_vertical_layout()
{
}

void FloatingContainer::toggle_layout()
{
}

void FloatingContainer::restore_state(MirWindowState state)
{
    restore_state_ = state;
}

std::optional<MirWindowState> FloatingContainer::restore_state()
{
    auto state = restore_state_;
    restore_state_.reset();
    return state;
}

Workspace *FloatingContainer::get_workspace() const
{
    return workspace_;
}

Output *FloatingContainer::get_output() const
{
    return workspace_->get_output();
}

glm::mat4 FloatingContainer::get_transform() const
{
    return transform;
}

void FloatingContainer::set_transform(glm::mat4 transform_)
{
    transform = transform_;
}

uint32_t FloatingContainer::animation_handle() const
{
    return animation_handle_;
}

void FloatingContainer::animation_handle(uint32_t handle)
{
    animation_handle_ = handle;
}

bool FloatingContainer::is_focused() const
{
    return get_output()->get_state().active_window == window_;
}

ContainerType FloatingContainer::get_type() const
{
    return ContainerType::floating;
}

glm::mat4 FloatingContainer::get_workspace_transform() const
{
    if (is_pinned)
        return glm::mat4(1.f);

    auto output = get_output();
    auto workspace = get_workspace();
    auto const workspace_rect = output->get_workspace_rectangle(workspace->get_workspace());
    return glm::translate(
        glm::vec3(workspace_rect.top_left.x.as_int(), workspace_rect.top_left.y.as_int(), 0));
}

glm::mat4 FloatingContainer::get_output_transform() const
{
    if (is_pinned)
        return glm::mat4(1.f);

    return get_output()->get_transform();
}

bool FloatingContainer::select_next(Direction)
{
    return false;
}

bool FloatingContainer::move(Direction direction)
{
    return move_by(direction, 10);
}

bool FloatingContainer::move_by(Direction direction, int pixels)
{
    auto& info = window_controller.info_for(window_);
    auto prev_pos = window_.top_left();
    miral::WindowSpecification spec;
    switch (direction)
    {
    case Direction::down:
        spec.top_left() = {
        prev_pos.x.as_int(), prev_pos.y.as_int() + pixels
        };
        break;
    case Direction::up:
        spec.top_left() = {
        prev_pos.x.as_int(), prev_pos.y.as_int() - pixels
        };
        break;
    case Direction::left:
        spec.top_left() = {
        prev_pos.x.as_int() - pixels, prev_pos.y.as_int()
        };
        break;
    case Direction::right:
        spec.top_left() = {
        prev_pos.x.as_int() + pixels, prev_pos.y.as_int()
        };
        break;
    default:
        mir::log_warning("Unknown direction to move_active_window_by_amount: %d\n", (int)direction);
        return false;
    }

    window_controller.modify(info.window(), spec);
    return true;
}

bool FloatingContainer::move_to(int x, int y)
{
    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    window_controller.modify(window_, spec);
    return true;
}

std::weak_ptr<ParentContainer> FloatingContainer::get_parent() const
{
    return std::weak_ptr<ParentContainer>();
}
