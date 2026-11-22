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

#ifndef MIRACLE_WM_MATH_HELPERS_H
#define MIRACLE_WM_MATH_HELPERS_H

#include "tessellation_helpers.h"

#include <mir/geometry/rectangle.h>
#include <optional>

namespace miracle
{
template <typename T>
float as_float(T const& value)
{
    return static_cast<float>(value.as_int());
}

std::optional<mir::geometry::Rectangle> intersect(
    const mir::geometry::Rectangle& a, const mir::geometry::Rectangle& b);

inline mir::gl::RectF to_rect_f(mir::geometry::Rectangle const& rect)
{
    return mir::gl::RectF {
        static_cast<float>(rect.top_left.x.as_int()),
        static_cast<float>(rect.top_left.y.as_int()),
        static_cast<float>(rect.size.width.as_int()),
        static_cast<float>(rect.size.height.as_int())
    };
}

/// Maps \p rect, which is in the coordinates of \p src, into \p dst.
/// \p src must not be empty.
inline mir::gl::RectF map(
    mir::gl::RectF const& rect, mir::gl::RectF const& src, mir::gl::RectF const& dst)
{
    float const kx = dst.width / src.width;
    float const ky = dst.height / src.height;
    return mir::gl::RectF {
        dst.x + (rect.x - src.x) * kx,
        dst.y + (rect.y - src.y) * ky,
        rect.width * kx,
        rect.height * ky
    };
}

}

#endif // MIRACLE_WM_MATH_HELPERS_H
