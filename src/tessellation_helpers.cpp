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

#include "tessellation_helpers.h"
#include "geometry_helpers.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/renderable.h"

#include <algorithm>
#include <cmath>

namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace geom = mir::geometry;

namespace
{
struct SrcTexCoords
{
    GLfloat top;
    GLfloat bottom;
    GLfloat left;
    GLfloat right;
};

auto tex_coords_from_rect(geom::Size buffer_size, geom::RectangleD sample_rect) -> SrcTexCoords
{
    /* GL Texture coordinates are normalised to the size of the buffer, so (0.0, 0.0) is the top-left
     * and (1.0, 1.0) is the bottom-right
     */
    SrcTexCoords coords;
    coords.top = static_cast<GLfloat>(sample_rect.top() / buffer_size.height);
    coords.bottom = static_cast<GLfloat>(sample_rect.bottom() / buffer_size.height);
    coords.left = static_cast<GLfloat>(sample_rect.left() / buffer_size.width);
    coords.right = static_cast<GLfloat>(sample_rect.right() / buffer_size.width);
    return coords;
}
}

auto mgl::stretch_scale(geom::Size const& source_size, geom::Size const& target_size) -> mgl::StretchScale
{
    using namespace miracle::geometry_helpers::gl;
    if (width(source_size) <= 0.f || height(source_size) <= 0.f)
        return { 1.f, 1.f };

    return { width(target_size) / width(source_size), height(target_size) / height(source_size) };
}

auto mgl::committed_window_rect(
    geom::Point const& window_top_left,
    geom::Size const& presented_size,
    geom::Displacement const& inset) -> geom::Rectangle
{
    // Floored at 1: an inset larger than the buffer - a client that shrank its shadow while
    // a resize was in flight - must not produce an empty or inverted rectangle for
    // stretch_scale to divide by.
    return {
        window_top_left,
        geom::Size {
                    std::max((presented_size.width - inset.dx).as_int(), 1),
                    std::max((presented_size.height - inset.dy).as_int(), 1) }
    };
}

namespace
{
/// The body of tessellate_renderable_into_rectangle, spelled out in the three values it
/// reads off the renderable.
mgl::Primitive tessellate_into_rectangle(
    geom::Rectangle const& screen_position,
    geom::Size const& buffer_size,
    geom::RectangleD const& src_bounds,
    geom::Displacement const& offset,
    bool const is_flipped,
    std::optional<geom::Rectangle> const& clip_area,
    std::optional<mgl::Stretch> const& stretch)
{
    using namespace miracle::geometry_helpers::gl;
    auto const& rect = screen_position;

    GLfloat window_left = x(rect.top_left) - static_cast<GLfloat>(offset.dx.as_int());
    GLfloat window_top = y(rect.top_left) - static_cast<GLfloat>(offset.dy.as_int());
    GLfloat window_width = width(rect.size);
    GLfloat window_height = height(rect.size);

    GLfloat const clip_left = clip_area
        ? x(clip_area->top_left) - static_cast<GLfloat>(offset.dx.as_int())
        : 0.f;
    GLfloat const clip_top = clip_area
        ? y(clip_area->top_left) - static_cast<GLfloat>(offset.dy.as_int())
        : 0.f;

    if (stretch && clip_area)
    {
        /* A resize animation asks for the content at a size the client is not drawing at
         * yet, so the committed window rectangle - the same one the border is sized from -
         * is scaled to that size at the clip's top-left and the renderable is carried
         * along by that map. See Stretch for why the renderable keeps its own offset and
         * size; the crop below trims whatever ends up outside the clip.
         */
        auto const [scale_x, scale_y] = mgl::stretch_scale(stretch->source.size, stretch->target);

        window_left = clip_left + (x(rect.top_left) - x(stretch->source.top_left)) * scale_x;
        window_top = clip_top + (y(rect.top_left) - y(stretch->source.top_left)) * scale_y;
        window_width *= scale_x;
        window_height *= scale_y;
    }

    mgl::Primitive rectangle;
    rectangle.type = GL_TRIANGLE_STRIP;

    auto const [src_top, src_bottom, src_left, src_right] = tex_coords_from_rect(buffer_size, src_bounds);

    /* The clip area is a sub-rectangle of the quad. Rather than clipping in framebuffer
     * space, we express it as the fraction of the quad that survives, then shrink both the
     * quad and the sampled texture range to that fraction. The clipped geometry then goes
     * through the vertex shader's usual transform chain, so output rotation, surface layout
     * and the workspace transform all apply to it for free.
     */
    GLfloat fx0 = 0.f, fx1 = 1.f, fy0 = 0.f, fy1 = 1.f;
    if (clip_area && window_width > 0.f && window_height > 0.f)
    {
        fx0 = std::clamp((clip_left - window_left) / window_width, 0.f, 1.f);
        fx1 = std::clamp((clip_left + width(clip_area->size) - window_left) / window_width, 0.f, 1.f);
        fy0 = std::clamp((clip_top - window_top) / window_height, 0.f, 1.f);
        fy1 = std::clamp((clip_top + height(clip_area->size) - window_top) / window_height, 0.f, 1.f);
    }

    GLfloat const left = window_left + fx0 * window_width;
    GLfloat const right = window_left + fx1 * window_width;
    GLfloat const top = window_top + fy0 * window_height;
    GLfloat const bottom = window_top + fy1 * window_height;

    // Narrow the source range to the same fraction the quad was cut down to. This happens
    // before the vertical flip below so that a flipped texture still clips at the edge the
    // clip area actually names.
    GLfloat const tex_left = std::lerp(src_left, src_right, fx0);
    GLfloat const tex_right = std::lerp(src_left, src_right, fx1);
    GLfloat const tex_top = std::lerp(src_top, src_bottom, fy0);
    GLfloat const tex_bottom = std::lerp(src_top, src_bottom, fy1);

    auto& vertices = rectangle.vertices;
    vertices[0] = {
        { left, top, 0.0f },
        { tex_left, is_flipped ? 1.f - tex_top : tex_top }
    };
    vertices[1] = {
        { left, bottom, 0.0f },
        { tex_left, is_flipped ? 1.f - tex_bottom : tex_bottom }
    };
    vertices[2] = {
        { right, top, 0.0f },
        { tex_right, is_flipped ? 1.f - tex_top : tex_top }
    };
    vertices[3] = {
        { right, bottom, 0.0f },
        { tex_right, is_flipped ? 1.f - tex_bottom : tex_bottom }
    };
    return rectangle;
}
}

mgl::Primitive mgl::tessellate_renderable_into_rectangle(
    mg::Renderable const& renderable,
    geom::Displacement const& offset,
    bool const is_flipped,
    std::optional<geom::Rectangle> const& clip_area,
    std::optional<Stretch> const& stretch)
{
    return ::tessellate_into_rectangle(
        renderable.screen_position(), renderable.buffer()->size(), renderable.src_bounds(),
        offset, is_flipped, clip_area, stretch);
}
