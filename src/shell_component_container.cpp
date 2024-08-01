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

#include "shell_component_container.h"
#include "window_controller.h"

namespace miracle
{

ShellComponentContainer::ShellComponentContainer(
    miral::Window const& window_,
    miracle::WindowController& window_controller)
    : window_{window_},
      window_controller{window_controller},
      Container(nullptr)
{
}

void ShellComponentContainer::restore_state(MirWindowState state)
{

}

std::optional<MirWindowState> ShellComponentContainer::restore_state()
{
    return std::optional<MirWindowState>();
}

void ShellComponentContainer::commit_changes()
{

}

mir::geometry::Rectangle ShellComponentContainer::get_logical_area() const
{
    return mir::geometry::Rectangle();
}

void ShellComponentContainer::set_logical_area(mir::geometry::Rectangle const &rectangle)
{

}

mir::geometry::Rectangle ShellComponentContainer::get_visible_area() const
{
    return mir::geometry::Rectangle();
}

void ShellComponentContainer::constrain()
{

}

void ShellComponentContainer::set_parent(std::shared_ptr<ParentContainer> const &ptr)
{

}

size_t ShellComponentContainer::get_min_height() const
{
    return 0;
}

size_t ShellComponentContainer::get_min_width() const
{
    return 0;
}

void ShellComponentContainer::handle_ready()
{

}

void ShellComponentContainer::handle_modify(miral::WindowSpecification const& specification)
{
    window_controller.modify(window_, specification);
}

void ShellComponentContainer::handle_request_move(MirInputEvent const *input_event)
{

}

void ShellComponentContainer::handle_request_resize(MirInputEvent const *input_event, MirResizeEdge edge)
{

}

void ShellComponentContainer::handle_raise()
{

}

bool ShellComponentContainer::resize(Direction direction)
{
    return false;
}

bool ShellComponentContainer::toggle_fullscreen()
{
    return false;
}

void ShellComponentContainer::request_horizontal_layout()
{

}

void ShellComponentContainer::request_vertical_layout()
{

}

void ShellComponentContainer::toggle_layout()
{

}

void ShellComponentContainer::on_focus_gained()
{

}

void ShellComponentContainer::on_focus_lost()
{

}

void ShellComponentContainer::on_move_to(mir::geometry::Point const &top_left)
{

}

mir::geometry::Rectangle
ShellComponentContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const &rectangle)
{
    return mir::geometry::Rectangle();
}

Workspace *ShellComponentContainer::get_workspace() const
{
    return nullptr;
}

Output *ShellComponentContainer::get_output() const
{
    return nullptr;
}

glm::mat4 ShellComponentContainer::get_transform() const
{
    return glm::mat4();
}

void ShellComponentContainer::set_transform(glm::mat4 transform)
{

}

glm::mat4 ShellComponentContainer::get_workspace_transform() const
{
    return glm::mat4();
}

glm::mat4 ShellComponentContainer::get_output_transform() const
{
    return glm::mat4();
}

uint32_t ShellComponentContainer::animation_handle() const
{
    return 0;
}

void ShellComponentContainer::animation_handle(uint32_t uint_32)
{

}

bool ShellComponentContainer::is_focused() const
{
    return false;
}

ContainerType ShellComponentContainer::get_type() const
{
    return ContainerType::shell;
}

void ShellComponentContainer::on_open()
{

}

std::optional<miral::Window> ShellComponentContainer::window() const
{
    return window_;
}
} // miracle