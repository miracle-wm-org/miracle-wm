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

    /// The scale a stretch maps \p source_size onto \p target_size with. (1, 1) when the two
    /// match, so an unstretched quad is left exactly where it was. Both the quad and the
    /// 'surfaceSize' uniform are derived from this, so they cannot disagree. The target is
    /// not necessarily the clip: a resize animation clamps the clip to what the client can
    /// actually reach before stretching onto it.
    struct StretchScale
    {
        float x;
        float y;
    };

    auto stretch_scale(geometry::Size const& source_size, geometry::Size const& target_size) -> StretchScale;

    /// How far a client's committed buffer overshoots the content size it was given: a
    /// CSD client draws its own drop shadow into the buffer and tells the compositor, via
    /// xdg window geometry, that only the inner rectangle is the window. Zero for a client
    /// whose buffer is its content, which is everything server-side decorated.
    ///
    /// Only meaningful on a frame where the client is caught up. During a resize the
    /// buffer and the content size sit on opposite sides of the client's ack latency, so
    /// their difference is the animation delta rather than the shadow - which is why the
    /// renderer measures this while a window is settled and remembers it for the animated
    /// frames rather than recomputing it per frame.
    auto shadow_band(
        geometry::Size const& presented_size,
        geometry::Size const& content_size) -> geometry::Displacement;

    /// The window rectangle the buffer a client has actually committed corresponds to.
    ///
    /// A resize animation asks the client for its final size on the first frame, but the
    /// renderable the compositor hands us is a snapshot of the last buffer the client
    /// committed, which for a frame or more is still the pre-resize one. \p window_size is
    /// live and has already moved, so it is not what that buffer was drawn for - but
    /// \p window_size minus \p content_size is the margin the compositor itself set, which
    /// does not change across a resize and is read from the same live surface, so the two
    /// are mutually consistent whatever the client has done. Taking \p shadow off the
    /// committed buffer's own size gives the content size that buffer was drawn for, and
    /// adding that margin back turns it into a window size - exactly, with no guessing,
    /// caught up or not.
    auto natural_window_rect(
        geometry::Point const& window_top_left,
        geometry::Size const& presented_size,
        geometry::Size const& window_size,
        geometry::Size const& content_size,
        geometry::Displacement const& shadow) -> geometry::Rectangle;

    /// Builds the quad for \p renderable. \p clip_area is always a crop: the quad is cut down
    /// to that rectangle and the texture coordinates are narrowed to the matching fraction of
    /// the source, so the clipped-away region is never sampled.
    ///
    /// \p stretch_size does not change that; it changes what is being cropped. With it set,
    /// \p source_rect - the natural window rectangle, the one the border is sized from - is
    /// scaled to \p stretch_size at the clip's top-left, and the renderable is carried along by
    /// that same affine map. The renderable's own rectangle is kept, so a client whose buffer is
    /// bigger than its window (a CSD shadow margin) or smaller (a server-side decoration inset)
    /// lands proportionally inside the clip rather than being smeared across all of it. Whatever
    /// spills outside the clip afterwards - the shadow band - is then cropped away as usual.
    ///
    /// \p stretch_size is not the clip. It is the size the client can actually be drawn at,
    /// which a resize animation freezes at the client's minimum (or maximum) once the clip
    /// passes it. A quad wider than the clip is cropped by it; a narrower one simply leaves the
    /// rest of the clip empty.
    ///
    /// When \p source_rect is unset the renderable's own rectangle stands in for it, which
    /// makes the map scale the whole renderable to \p stretch_size.
    Primitive tessellate_renderable_into_rectangle(
        graphics::Renderable const& renderable,
        geometry::Displacement const& offset,
        bool const is_flipped,
        std::optional<geometry::Rectangle> const& clip_area,
        std::optional<geometry::Size> const& stretch_size = std::nullopt,
        std::optional<geometry::Rectangle> const& source_rect = std::nullopt);

    /// The same geometry, spelled out rather than read off a renderable. A cross-fade draws
    /// a buffer that was captured frames ago and has no live renderable to ask, so the three
    /// values a renderable would have supplied - where it sits, how big its buffer is, and
    /// which part of that buffer it draws - are passed directly. Sharing one body with the
    /// overload above is what keeps the two layers of a fade registered with each other:
    /// they cannot drift if there is only one map.
    Primitive tessellate_into_rectangle(
        geometry::Rectangle const& screen_position,
        geometry::Size const& buffer_size,
        geometry::RectangleD const& src_bounds,
        geometry::Displacement const& offset,
        bool const is_flipped,
        std::optional<geometry::Rectangle> const& clip_area,
        std::optional<geometry::Size> const& stretch_size = std::nullopt,
        std::optional<geometry::Rectangle> const& source_rect = std::nullopt);

}
}
#endif /* MIR_GL_TESSELLATION_HELPERS_H_ */
