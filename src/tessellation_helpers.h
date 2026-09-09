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

    /// The scale a stretch maps \p source_size onto \p target_size with. (1, 1) when the
    /// two match. Both the quad and the 'surfaceSize' uniform are derived from this, so
    /// they cannot disagree.
    struct StretchScale
    {
        float x;
        float y;
    };

    auto stretch_scale(geometry::Size const& source_size, geometry::Size const& target_size) -> StretchScale;

    /// What a resize animation asks be done to a quad: [source] - the natural window
    /// rectangle, the one the border is sized from - is scaled onto [target] at the clip's
    /// top-left, and the renderable is carried along by that same affine map. Keeping the
    /// renderable's own rectangle is what lands a buffer bigger than its window (a CSD
    /// shadow margin) or smaller (a decoration inset) proportionally inside the clip
    /// rather than smeared across all of it.
    ///
    /// [target] is not the clip: it is the size the client can actually be drawn at, which
    /// a resize freezes at the client's limit once the clip passes it. A quad wider than
    /// the clip is cropped by it; a narrower one leaves the rest of it empty.
    struct Stretch
    {
        geometry::Size target;
        geometry::Rectangle source;
    };

    /// How far a client's committed buffer overshoots the content size it was given: a CSD
    /// client draws its own drop shadow into the buffer and declares, via xdg window
    /// geometry, that only the inner rectangle is the window. Zero for everything else.
    ///
    /// Only meaningful on a frame where the client is caught up: during a resize the two
    /// sizes sit on opposite sides of the client's ack latency, so their difference is the
    /// animation delta rather than the shadow.
    auto shadow_band(
        geometry::Size const& presented_size,
        geometry::Size const& content_size) -> geometry::Displacement;

    /// The window rectangle the buffer a client has actually committed corresponds to.
    ///
    /// A resize asks the client for its final size on the first frame, but the renderable
    /// is a snapshot of the last buffer it committed, which for a frame or more is still
    /// the pre-resize one - so \p window_size, which is live, is not what that buffer was
    /// drawn for. Their difference is: \p window_size minus \p content_size is the margin
    /// the compositor itself set, which does not change across a resize. Taking \p shadow
    /// off the committed buffer's size gives the content size it was drawn for, and adding
    /// that margin back turns it into a window size - exactly, with no guessing.
    auto natural_window_rect(
        geometry::Point const& window_top_left,
        geometry::Size const& presented_size,
        geometry::Size const& window_size,
        geometry::Size const& content_size,
        geometry::Displacement const& shadow) -> geometry::Rectangle;

    /// Builds the quad for \p renderable. \p clip_area is always a crop: the quad is cut
    /// down to that rectangle and the texture coordinates are narrowed to the matching
    /// fraction of the source, so the clipped-away region is never sampled. \p stretch does
    /// not change that; it changes what is being cropped - see Stretch.
    Primitive tessellate_renderable_into_rectangle(
        graphics::Renderable const& renderable,
        geometry::Displacement const& offset,
        bool const is_flipped,
        std::optional<geometry::Rectangle> const& clip_area,
        std::optional<Stretch> const& stretch = std::nullopt);

}
}
#endif /* MIR_GL_TESSELLATION_HELPERS_H_ */
