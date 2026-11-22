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

#ifndef MIR_GL_TESSELLATION_HELPERS_H_
#define MIR_GL_TESSELLATION_HELPERS_H_
#include "mir/geometry/displacement.h"
#include "mir/geometry/rectangle.h"
#include "primitive.h"

#include <optional>

namespace mir
{
namespace graphics
{
    class Renderable;
}
namespace gl
{

    /// Builds the quad for \p renderable. When \p clip_area is set, the quad is cut down to
    /// that sub-rectangle of the window and the texture coordinates are narrowed to the
    /// matching fraction of the source, so the clipped-away region is never sampled.
    Primitive tessellate_renderable_into_rectangle(
        graphics::Renderable const& renderable,
        geometry::Displacement const& offset,
        bool const is_flipped,
        std::optional<geometry::Rectangle> const& clip_area);

    /// A rectangle in screen coordinates with fractional position and size.
    struct RectF
    {
        float x = 0.f;
        float y = 0.f;
        float width = 0.f;
        float height = 0.f;
    };

    /// Builds the quad that draws the \p src_bounds region of a \p buffer_size texture
    /// into \p dest. \p clip_area behaves as it does for
    /// [tessellate_renderable_into_rectangle], but is relative to \p dest, which need not
    /// be the size of the buffer: the texture is stretched to fill it.
    Primitive tessellate_texture_into_rectangle(
        RectF const& dest,
        geometry::Size const& buffer_size,
        geometry::RectangleD const& src_bounds,
        bool const is_flipped,
        std::optional<geometry::Rectangle> const& clip_area);
}
}
#endif /* MIR_GL_TESSELLATION_HELPERS_H_ */
