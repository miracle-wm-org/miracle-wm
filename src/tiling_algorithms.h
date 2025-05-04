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

#ifndef TILING_ALGORITHMS_H
#define TILING_ALGORITHMS_H
#include <vector>

namespace miracle
{
struct TilePosition
{
    double size;
    double position;
};

struct TileInsert
{
    std::vector<TilePosition> const positions;
};

TileInsert insert_node(
    std::vector<TilePosition> const& existing_positions,
    double const total_container_size,
    double const container_start,
    size_t const insertion_index);
}

#endif // TILING_ALGORITHMS_H
