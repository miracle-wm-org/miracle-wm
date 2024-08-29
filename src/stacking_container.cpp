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

#include "stacking_container.h"

namespace miracle
{
ContainerType StackingContainer::get_type() const
{
    return ContainerType::group;
}

void StackingContainer::show()
{

}

void StackingContainer::hide()
{

}

void StackingContainer::commit_changes()
{

}

mir::geometry::Rectangle StackingContainer::get_logical_area() const
{
    return mir::geometry::Rectangle();
}

void StackingContainer::set_logical_area(mir::geometry::Rectangle const &rectangle)
{

}

mir::geometry::Rectangle StackingContainer::get_visible_area() const
{
    return mir::geometry::Rectangle();
}

void StackingContainer::constrain()
{

}

std::weak_ptr<ParentContainer> StackingContainer::get_parent() const
{
    return std::weak_ptr<ParentContainer>();
}

void StackingContainer::set_parent(std::shared_ptr<ParentContainer> const &ptr)
{

}

size_t StackingContainer::get_min_height() const
{
    return 0;
}

size_t StackingContainer::get_min_width() const
{
    return 0;
}

void StackingContainer::handle_ready()
{

}

void StackingContainer::handle_modify(miral::WindowSpecification const &specification)
{

}

void StackingContainer::handle_request_move(MirInputEvent const *input_event)
{

}

void StackingContainer::handle_request_resize(MirInputEvent const *input_event, MirResizeEdge edge)
{

}

void StackingContainer::handle_raise()
{

}

bool StackingContainer::resize(Direction direction)
{
    return false;
}

bool StackingContainer::toggle_fullscreen()
{
    return false;
}

void StackingContainer::request_horizontal_layout()
{

}

void StackingContainer::request_vertical_layout()
{

}

void StackingContainer::toggle_layout()
{

}

void StackingContainer::on_open()
{

}

void StackingContainer::on_focus_gained()
{

}

void StackingContainer::on_focus_lost()
{

}

void StackingContainer::on_move_to(mir::geometry::Point const &top_left)
{

}

mir::geometry::Rectangle
StackingContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const &rectangle)
{
    return mir::geometry::Rectangle();
}

Workspace *StackingContainer::get_workspace() const
{
    return nullptr;
}

Output *StackingContainer::get_output() const
{
    return nullptr;
}

glm::mat4 StackingContainer::get_transform() const
{
    return glm::mat4();
}

void StackingContainer::set_transform(glm::mat4 transform)
{

}

glm::mat4 StackingContainer::get_workspace_transform() const
{
    return Container::get_workspace_transform();
}

glm::mat4 StackingContainer::get_output_transform() const
{
    return Container::get_output_transform();
}

uint32_t StackingContainer::animation_handle() const
{
    return 0;
}

void StackingContainer::animation_handle(uint32_t uint_32)
{

}

bool StackingContainer::is_focused() const
{
    return false;
}

bool StackingContainer::is_fullscreen() const
{
    return false;
}

std::optional<miral::Window> StackingContainer::window() const
{
    return std::optional<miral::Window>();
}

bool StackingContainer::select_next(Direction direction)
{
    return false;
}

bool StackingContainer::pinned() const
{
    return false;
}

bool StackingContainer::pinned(bool b)
{
    return false;
}

bool StackingContainer::move(Direction direction)
{
    return false;
}

bool StackingContainer::move_by(Direction direction, int pixels)
{
    return false;
}

bool StackingContainer::move_to(int x, int y)
{
    return false;
}

bool StackingContainer::toggle_stacked()
{
    return false;
}

bool StackingContainer::is_stacking() const
{
    return false;
}
} // miracle