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

#include "tiling_algorithms.h"
#include <cmath>

miracle::TileInsert miracle::insert_node(
    std::vector<TilePosition> const& existing_positions,
    double const total_container_size,
    double const container_start,
    size_t const insertion_index)
{
    std::vector<TilePosition> new_positions;
    if (existing_positions.empty())
    {
        new_positions.push_back({ .size = total_container_size,
            .position = container_start });
        return { new_positions };
    }

    double const requested_item_size = floor(total_container_size / static_cast<double>(existing_positions.size() + 1));
    double total_size_used = 0;

    double next_position = container_start;
    for (size_t i = 0; i < existing_positions.size(); i++)
    {
        if (i == insertion_index)
        {
            new_positions.push_back({ .size = requested_item_size,
                .position = next_position });
            next_position = new_positions.back().position + new_positions.back().size;
            total_size_used += requested_item_size;
        }

        double const percent_to_shrink = existing_positions[i].size / total_container_size;
        double const reduction = requested_item_size * percent_to_shrink;
        double const new_size = existing_positions[i].size - reduction;
        new_positions.push_back({ .size = new_size,
            .position = next_position });
        next_position = new_positions.back().position + new_positions.back().size;
        total_size_used += new_size;
    }

    if (insertion_index >= existing_positions.size())
    {
        new_positions.push_back({ .size = requested_item_size,
            .position = next_position });
        total_size_used += requested_item_size;
    }

    /// If we have any leftover size, we're going to give it all to the last
    double const remaining_size = total_container_size - total_size_used;
    new_positions.back().size += remaining_size;

    return { new_positions };
}