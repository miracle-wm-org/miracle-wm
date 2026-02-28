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

#include "collection_container.h"
#include "window_container.h"
#include <algorithm>

using namespace miracle;

void CollectionContainer::for_each_window(std::function<void(std::shared_ptr<WindowContainer>)> const& func) const
{
    for (auto const& child : children())
    {
        if (auto const wc = Container::as_window_container(child))
            func(wc);
        else if (auto const cc = dynamic_cast<CollectionContainer*>(child.get()))
            cc->for_each_window(func);
    }
}

size_t CollectionContainer::num_children() const
{
    return children().size();
}

std::shared_ptr<Container> CollectionContainer::at(size_t i) const
{
    if (i >= num_children())
        return nullptr;

    return children().at(i);
}