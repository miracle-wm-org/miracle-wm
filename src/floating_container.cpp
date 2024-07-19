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


#include "floating_container.h"
#include "leaf_container.h"

using namespace miracle;

FloatingContainer::FloatingContainer()
    : Container(nullptr)
{
}

void FloatingContainer::add_leaf(std::shared_ptr<LeafContainer> const& leaf)
{
    container = leaf;
}

void FloatingContainer::commit_changes()
{

}

mir::geometry::Rectangle FloatingContainer::get_logical_area() const
{

}

void FloatingContainer::set_logical_area(mir::geometry::Rectangle const& rectangle)
{

}

mir::geometry::Rectangle FloatingContainer::get_visible_area() const
{
    return mir::geometry::Rectangle();
}

void FloatingContainer::constrain() {}

void FloatingContainer::set_parent(std::shared_ptr<Container> const& parent)
{

}

size_t FloatingContainer::get_min_height() const
{
    return 1;
}

size_t FloatingContainer::get_min_width() const
{
    return 1;
}

bool FloatingContainer::pinned() const
{
    return is_pinned;
}

void FloatingContainer::pinned(bool in)
{
    is_pinned = in;
}

const miral::Window &FloatingContainer::window() const
{
    return container->get_window();
}