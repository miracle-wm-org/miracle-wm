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

/// Computes exposé-style placements for \p windows inside \p bounds by laying
/// them out on a balanced grid of `ceil(sqrt(n))` columns, centered in
/// \p bounds. Every window is centered in its own cell, and a short final row
/// is centered horizontally: three windows become two on top and one beneath,
/// five become three on top and two beneath.
///
/// Each window is scaled uniformly so its aspect ratio is preserved, and it is
/// always scaled down - a window that would fit its cell at full size is still
/// shrunk, so the spread reads as an overview of the desktop rather than a
/// rearrangement of it.
///
/// The placements are guaranteed to be non-overlapping, separated by at least
/// \p gap, and contained by \p bounds.
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
