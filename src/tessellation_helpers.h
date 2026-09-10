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

    /// What a resize animation asks be done to a quad: [source] - the window rectangle the
    /// committed buffer corresponds to, the one the border is sized from - is scaled onto
    /// [target] at the clip's top-left, and the renderable is carried along by that same
    /// affine map. Keeping the renderable's own rectangle is what lands a buffer bigger
    /// than its window (a CSD shadow margin) or smaller (a decoration inset)
    /// proportionally inside the clip rather than smeared across all of it.
    ///
    /// [target] is not the clip: it is the size the client can actually be drawn at, which
    /// a resize freezes at the client's limit once the clip passes it. A quad wider than
    /// the clip is cropped by it; a narrower one leaves the rest of it empty.
    struct Stretch
    {
        geometry::Size target;
        geometry::Rectangle source;
    };

    /// The window rectangle the buffer a client has actually committed corresponds to.
    ///
    /// A resize asks the client for its final size on the first frame, but the renderable is
    /// a snapshot of the last buffer it committed, which for a frame or more is still the
    /// pre-resize one - so the surface's live window size is not what that buffer was drawn
    /// for. \p presented_size is, once \p inset is taken off it.
    ///
    /// \p inset is how far the buffer overshoots the window it belongs to: positive for a
    /// CSD client that draws its own drop shadow past its xdg window geometry, negative for
    /// a server-decorated one whose buffer is only the content inside its frame. Either way
    /// it is a property of the client rather than of the animation, so it holds across a
    /// resize - which is what makes this exact rather than a guess.
    ///
    /// \p window_top_left is live, and deliberately so: the buffer is drawn relative to
    /// wherever the compositor has since moved the window. The renderable's own top-left is
    /// that same point displaced by the buffer's decoration inset and xdg geometry offset,
    /// and their difference is what carries both proportionally through the stretch.
    auto committed_window_rect(
        geometry::Point const& window_top_left,
        geometry::Size const& presented_size,
        geometry::Displacement const& inset) -> geometry::Rectangle;

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

    /// The same geometry, spelled out rather than read off a renderable: a cross-fade draws
    /// content captured frames ago and has no live renderable to ask. Sharing one body with
    /// the overload above is what keeps the two layers of a fade registered.
    Primitive tessellate_into_rectangle(
        geometry::Rectangle const& screen_position,
        geometry::Size const& buffer_size,
        geometry::RectangleD const& src_bounds,
        geometry::Displacement const& offset,
        bool const is_flipped,
        std::optional<geometry::Rectangle> const& clip_area,
        std::optional<Stretch> const& stretch = std::nullopt);

}
}
#endif /* MIR_GL_TESSELLATION_HELPERS_H_ */
