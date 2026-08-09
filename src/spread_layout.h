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

#ifndef SPREAD_LAYOUT_H
#define SPREAD_LAYOUT_H

#include <mir/geometry/rectangle.h>
#include <vector>

namespace miracle::spread_layout
{

/// Computes non-overlapping, exposé-style placements for \p windows inside
/// \p bounds by pushing the windows radially outward from the center of the
/// bounds and uniformly scaling them down until they all fit.
///
/// Deterministic: the same input always yields the same output.
///
/// \returns one rectangle per input window, in input order.
std::vector<mir::geometry::Rectangle> compute(
    mir::geometry::Rectangle const& bounds,
    std::vector<mir::geometry::Rectangle> const& windows,
    int gap = 16);

}

#endif
