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

#include "math_helpers.h"

std::optional<mir::geometry::Rectangle> miracle::intersect(
    const mir::geometry::Rectangle& a, const mir::geometry::Rectangle& b)
{
    int const ix1 = std::max(a.top_left.x.as_int(), b.top_left.x.as_int());
    int const iy1 = std::max(a.top_left.y.as_int(), b.top_left.y.as_int());
    int const ix2 = std::min(a.top_left.x.as_int() + a.size.width.as_int(), b.top_left.x.as_int() + b.size.width.as_int());
    int const iy2 = std::min(a.top_left.y.as_int() + a.size.height.as_int(), b.top_left.y.as_int() + b.size.height.as_int());

    if (ix2 > ix1 && iy2 > iy1)
    {
        return mir::geometry::Rectangle {
            { ix1,       iy1       },
            { ix2 - ix1, iy2 - iy1 }
        };
    }
    else
    {
        return std::nullopt;
    }
}