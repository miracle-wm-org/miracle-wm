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

#ifndef MIRACLE_TILING_ALGORITHMS_H
#define MIRACLE_TILING_ALGORITHMS_H
#include "container.h"
#include <vector>

namespace geom = mir::geometry;

namespace miracle
{
/// Insert a node in the provided \p containers at the \p insertion_index.
///
/// Callers should call #ParentContainer::commit after.
///
/// \tparam IsVertical whether or not the layout scheme is vertical
/// \param containers the containers in the group
/// \param container_area the available area for the group
/// \param insertion_index the desired insertion index.
/// \returns the rectangle describing the newly placed container
template <bool IsVertical>
inline geom::Rectangle insert_node(
    std::vector<std::shared_ptr<Container>> const& containers,
    geom::Rectangle const& container_area,
    size_t const insertion_index)
{
    if (containers.empty())
        return container_area;

    double const total_container_size = IsVertical ? container_area.size.height.as_int() : container_area.size.width.as_int();
    double const container_start = IsVertical ? container_area.top_left.y.as_int() : container_area.top_left.x.as_int();
    double const requested_item_size = floor(total_container_size / static_cast<double>(containers.size() + 1));
    double total_size_used = 0;

    double next_position = container_start;
    geom::Rectangle new_container_area;
    for (size_t i = 0; i < containers.size(); i++)
    {
        if (i == insertion_index)
        {
            geom::Point position = container_area.top_left;
            geom::Size size = container_area.size;
            if constexpr (IsVertical)
            {
                position.y = geom::Y { next_position };
                size.height = geom::Height { requested_item_size };
            }
            else
            {
                position.x = geom::X { next_position };
                size.width = geom::Width { requested_item_size };
            }
            new_container_area = geom::Rectangle { position, size };
            next_position = next_position + requested_item_size;
            total_size_used += requested_item_size;
        }

        auto const size = IsVertical ? containers[i]->get_logical_area().size.height.as_int()
                                     : containers[i]->get_logical_area().size.width.as_int();
        double const percent_to_shrink = size / total_container_size;
        double const reduction = requested_item_size * percent_to_shrink;
        double const new_size = size - reduction;
        auto const& container = containers[i];
        if constexpr (IsVertical)
        {
            container->set_logical_area({
                                            geom::Point {
                                                         container_area.top_left.x,
                                                         next_position },
                                            geom::Size {
                                                         container_area.size.width,
                                                         new_size      }
            },
                true);
        }
        else
        {
            container->set_logical_area({
                                            geom::Point {
                                                         next_position,
                                                         container_area.top_left.y,
                                                         },
                                            geom::Size {
                                                         new_size,
                                                         container_area.size.height,
                                                         }
            },
                true);
        }
        next_position = next_position + new_size;
        total_size_used += new_size;
    }

    /// If we have any leftover size, we're going to give it all to the last
    if (insertion_index >= containers.size())
    {
        geom::Point position = container_area.top_left;
        geom::Size size = container_area.size;
        total_size_used += requested_item_size;
        double const remaining_size = total_container_size - total_size_used;
        if constexpr (IsVertical)
        {
            position.y = geom::Y { next_position };
            size.height = geom::Height { requested_item_size + remaining_size };
        }
        else
        {
            position.x = geom::X { next_position };
            size.width = geom::Width { requested_item_size + remaining_size };
        }

        new_container_area = geom::Rectangle { position, size };
    }
    else
    {
        double const remaining_size = total_container_size - total_size_used;
        auto last_area = containers.back()->get_logical_area();
        if constexpr (IsVertical)
            last_area.size.height = geom::Height { last_area.size.height.as_int() + remaining_size };
        else
            last_area.size.width = geom::Width { last_area.size.width.as_int() + remaining_size };
        containers.back()->set_logical_area(last_area, true);
    }

    return new_container_area;
}
}

#endif // MIRACLE_TILING_ALGORITHMS_H
