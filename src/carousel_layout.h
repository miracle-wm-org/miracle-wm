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

#ifndef CAROUSEL_LAYOUT_H
#define CAROUSEL_LAYOUT_H

#include <mir/geometry/rectangle.h>
#include <vector>

namespace miracle::carousel_layout
{

/// Where a single window sits in the carousel.
///
/// Floating point because the carousel position is continuous while it
/// animates from one slot to the next.
struct Placement
{
    /// Top-left corner, in logical screen coordinates.
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;
    /// Multiplied into the window's alpha, so windows away from the center
    /// read as dimmer than the one that is up front.
    float opacity = 1.f;
};

/// Linearly interpolates every field of \p from towards \p to by \p p.
Placement lerp(Placement const& from, Placement const& to, float p);

/// True when (\p x, \p y) falls inside \p placement.
bool contains(Placement const& placement, float x, float y);

struct Options
{
    /// Width of the center slot's box, as a fraction of the bounds.
    float center_width_fraction = 0.30f;
    /// Height of the center slot's box, as a fraction of the bounds.
    float center_height_fraction = 0.62f;
    /// Every step away from the center multiplies the slot box by this.
    float side_scale = 0.78f;
    /// Opacity of a window a full step (or more) away from the center.
    float dim = 0.65f;
    /// Minimum horizontal gap between neighbouring windows.
    float gap = 16.f;
    /// A window is never drawn larger than this fraction of its real size.
    float max_scale = 1.f;
};

/// Lays \p windows out on a horizontal carousel inside \p bounds, with the
/// window at the (possibly fractional) index \p position front and center.
///
/// The strip has ends rather than wrapping: the windows before \p position run
/// off to the left and the ones after it run off to the right, and there is
/// nothing beyond either end.
///
/// Every window is scaled uniformly so its aspect ratio is preserved, fitted
/// into its slot's box, and never blown up past \p Options::max_scale. Slot
/// spacing is derived from the slot boxes rather than from the fitted window
/// sizes, which guarantees that no two placements overlap at any \p position.
///
/// Windows several steps from the center land entirely outside \p bounds and
/// are left there for the renderer to cull; callers do not need to trim the
/// result.
///
/// Deterministic: the same input always yields the same output.
///
/// \returns one placement per input window, in input order.
std::vector<Placement> compute(
    mir::geometry::Rectangle const& bounds,
    std::vector<mir::geometry::Rectangle> const& windows,
    float position,
    Options const& options = {});

}

#endif
