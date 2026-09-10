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

#include "stub_renderable.h"
#include "tessellation_helpers.h"
#include <cmath>
#include <gtest/gtest.h>
#include <optional>
#include <vector>

using namespace miracle::test;
namespace geom = mir::geometry;
namespace mgl = mir::gl;

namespace
{
/// The quad's bounds, read back off the triangle strip's corner vertices.
struct QuadBounds
{
    GLfloat left, top, right, bottom;
};

QuadBounds bounds_of(mgl::Primitive const& p)
{
    return { p.vertices[0].position[0], p.vertices[0].position[1],
        p.vertices[3].position[0], p.vertices[3].position[1] };
}

/// The sampled texture range, likewise.
struct TexBounds
{
    GLfloat left, top, right, bottom;
};

TexBounds tex_of(mgl::Primitive const& p)
{
    return { p.vertices[0].texcoord[0], p.vertices[0].texcoord[1],
        p.vertices[3].texcoord[0], p.vertices[3].texcoord[1] };
}
}

TEST(TessellationHelpersTest, AnUnclippedQuadIsTheWholeWindowAndTheWholeSource)
{
    StubRenderable renderable({
                                  { 10,  20  },
                                  { 800, 600 }
    },
        { 800, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, std::nullopt);

    auto const b = bounds_of(quad);
    EXPECT_FLOAT_EQ(b.left, 10.f);
    EXPECT_FLOAT_EQ(b.top, 20.f);
    EXPECT_FLOAT_EQ(b.right, 810.f);
    EXPECT_FLOAT_EQ(b.bottom, 620.f);

    auto const t = tex_of(quad);
    EXPECT_FLOAT_EQ(t.left, 0.f);
    EXPECT_FLOAT_EQ(t.top, 0.f);
    EXPECT_FLOAT_EQ(t.right, 1.f);
    EXPECT_FLOAT_EQ(t.bottom, 1.f);
}

TEST(TessellationHelpersTest, AClipCutsBothTheQuadAndTheSampledRange)
{
    StubRenderable renderable({
                                  { 0,   0   },
                                  { 800, 600 }
    },
        { 800, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 0, 0 }, { 400, 600 } });

    auto const b = bounds_of(quad);
    EXPECT_FLOAT_EQ(b.right, 400.f);

    // Half the window survives, so half the source is sampled.
    auto const t = tex_of(quad);
    EXPECT_FLOAT_EQ(t.right, 0.5f);
}

TEST(TessellationHelpersTest, StretchingMakesTheQuadTheClipAndSamplesTheWholeSource)
{
    // The client has already adopted the final 400px size while the animated clip
    // is still at 700: the crop path could only ever reach the window's own 400.
    StubRenderable renderable({
                                  { 0,   0   },
                                  { 400, 600 }
    },
        { 400, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 0, 0 }, { 700, 600 } }, mgl::Stretch { geom::Size { 700, 600 }, renderable.screen_position() });

    auto const b = bounds_of(quad);
    EXPECT_FLOAT_EQ(b.left, 0.f);
    EXPECT_FLOAT_EQ(b.right, 700.f);
    EXPECT_FLOAT_EQ(b.bottom, 600.f);

    // Nothing is cropped away: the whole buffer is scaled across the clip.
    auto const t = tex_of(quad);
    EXPECT_FLOAT_EQ(t.left, 0.f);
    EXPECT_FLOAT_EQ(t.right, 1.f);
    EXPECT_FLOAT_EQ(t.bottom, 1.f);
}

TEST(TessellationHelpersTest, AStretchedQuadTracksTheClipWhereverItSits)
{
    StubRenderable renderable({
                                  { 0,   0   },
                                  { 800, 600 }
    },
        { 800, 600 });

    // A resize that moves the left edge: the clip's top-left is not the window's.
    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 100, 50 }, { 300, 200 } }, mgl::Stretch { geom::Size { 300, 200 }, renderable.screen_position() });

    auto const b = bounds_of(quad);
    EXPECT_FLOAT_EQ(b.left, 100.f);
    EXPECT_FLOAT_EQ(b.top, 50.f);
    EXPECT_FLOAT_EQ(b.right, 400.f);
    EXPECT_FLOAT_EQ(b.bottom, 250.f);
}

TEST(TessellationHelpersTest, StretchingWithoutAClipFallsBackToTheWholeWindow)
{
    StubRenderable renderable({
                                  { 0,   0   },
                                  { 800, 600 }
    },
        { 800, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, std::nullopt, mgl::Stretch { geom::Size { 300, 200 }, renderable.screen_position() });

    auto const b = bounds_of(quad);
    EXPECT_FLOAT_EQ(b.right, 800.f);
    EXPECT_FLOAT_EQ(b.bottom, 600.f);
}

namespace
{
/// Every corner of the strip, so a remap that transposes x and y cannot slip through
/// a check on two of them.
void expect_quad(mgl::Primitive const& p, GLfloat left, GLfloat top, GLfloat right, GLfloat bottom)
{
    EXPECT_FLOAT_EQ(p.vertices[0].position[0], left);
    EXPECT_FLOAT_EQ(p.vertices[0].position[1], top);
    EXPECT_FLOAT_EQ(p.vertices[1].position[0], left);
    EXPECT_FLOAT_EQ(p.vertices[1].position[1], bottom);
    EXPECT_FLOAT_EQ(p.vertices[2].position[0], right);
    EXPECT_FLOAT_EQ(p.vertices[2].position[1], top);
    EXPECT_FLOAT_EQ(p.vertices[3].position[0], right);
    EXPECT_FLOAT_EQ(p.vertices[3].position[1], bottom);
}

void expect_tex(mgl::Primitive const& p, GLfloat left, GLfloat top, GLfloat right, GLfloat bottom)
{
    EXPECT_FLOAT_EQ(p.vertices[0].texcoord[0], left);
    EXPECT_FLOAT_EQ(p.vertices[0].texcoord[1], top);
    EXPECT_FLOAT_EQ(p.vertices[1].texcoord[0], left);
    EXPECT_FLOAT_EQ(p.vertices[1].texcoord[1], bottom);
    EXPECT_FLOAT_EQ(p.vertices[2].texcoord[0], right);
    EXPECT_FLOAT_EQ(p.vertices[2].texcoord[1], top);
    EXPECT_FLOAT_EQ(p.vertices[3].texcoord[0], right);
    EXPECT_FLOAT_EQ(p.vertices[3].texcoord[1], bottom);
}
}

TEST(TessellationHelpersTest, AStretchedShadowedClientPutsItsContentOnTheClip)
{
    // A CSD client - Chrome, Mattermost - sets an xdg window geometry inset from the
    // buffer it submits, so 40px of shadow surrounds the window on every side and the
    // renderable is bigger than, and offset from, the window itself.
    geom::Rectangle const window {
        { 100, 100 },
        { 800, 600 }
    };
    StubRenderable renderable({
                                  { 60,  60  },
                                  { 880, 680 }
    },
        { 880, 680 });

    // Half size: the animation is a third of the way through shrinking the window.
    geom::Rectangle const clip {
        { 100, 100 },
        { 400, 300 }
    };

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { clip.size, window });

    // The window maps onto the clip at 0.5, so the renderable maps to a 440x340 rectangle
    // starting 20px up and left of the clip - and the shadow band outside the clip is then
    // cropped away, leaving the content flush against the clip on all four sides.
    expect_quad(quad, 100.f, 100.f, 500.f, 400.f);

    // The surviving fraction is exactly the geometry sub-rect of the buffer: 40/880 in and
    // 840/880 out horizontally, 40/680 and 640/680 vertically.
    expect_tex(quad, 40.f / 880.f, 40.f / 680.f, 840.f / 880.f, 640.f / 680.f);
}

TEST(TessellationHelpersTest, AStretchedMarginKeepsItsInsetInsteadOfFillingTheClip)
{
    // Server-side decoration: miracle insets the content by border_size on every side,
    // so the renderable is the content rectangle, not the window rectangle.
    int const border_size = 10;
    geom::Rectangle const window {
        { 100, 100 },
        { 800, 600 }
    };
    StubRenderable renderable({
                                  { 110, 110 },
                                  { 780, 580 }
    },
        { 780, 580 });

    // Growing to double, so the inset should come out at border_size * 2.
    geom::Rectangle const clip {
        { 100,  100  },
        { 1600, 1200 }
    };

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { clip.size, window });

    auto const inset = static_cast<GLfloat>(border_size) * 2.f;
    expect_quad(quad, 100.f + inset, 100.f + inset, 1700.f - inset, 1300.f - inset);

    // The content sits wholly inside the clip, so nothing is cropped.
    expect_tex(quad, 0.f, 0.f, 1.f, 1.f);
}

TEST(TessellationHelpersTest, AStretchedUndecoratedClientStillFillsTheClipExactly)
{
    // No shadow and no margin: the renderable is the window, so the map takes it onto
    // the clip and the crop is a no-op - the behaviour the shadow case is a departure from.
    geom::Rectangle const window {
        { 0,   0   },
        { 400, 600 }
    };
    StubRenderable renderable(window, { 400, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 0, 0 }, { 700, 600 } }, mgl::Stretch { geom::Size { 700, 600 }, window });

    expect_quad(quad, 0.f, 0.f, 700.f, 600.f);
    expect_tex(quad, 0.f, 0.f, 1.f, 1.f);
}

TEST(TessellationHelpersTest, AStretchedSubRegionSamplesOnlyThatRegion)
{
    // A renderable that draws a sub-rect of an oversized buffer: the texcoord fraction the
    // crop narrows has to be taken against src_bounds, not against the whole buffer.
    geom::Rectangle const window {
        { 0,   0   },
        { 400, 400 }
    };
    StubRenderable renderable(window, {
                                          1024, 1024
    },
        geom::RectangleD { { 0, 0 }, { 512, 512 } });

    // Stretch to double, then crop the right half away.
    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 0, 0 }, { 800, 800 } }, mgl::Stretch { geom::Size { 800, 800 }, window });

    expect_quad(quad, 0.f, 0.f, 800.f, 800.f);
    // src_bounds is the top-left quarter of the buffer, so the whole of it is 0..0.5.
    expect_tex(quad, 0.f, 0.f, 0.5f, 0.5f);
}

TEST(TessellationHelpersTest, AStretchFrozenAtTheClientMinimumMatchesTheUnstretchedCrop)
{
    // gedit at its 300px minimum while the animated clip has already gone past it to
    // 200. The stretch is frozen at 300, so the scale is 1.0 and the quad has to come
    // out identical to the one the very next, un-animated frame draws with no stretch
    // at all. If these two disagree, the content pops when the animation completes.
    geom::Rectangle const window {
        { 600, 0   },
        { 300, 600 }
    };
    StubRenderable renderable(window, { 300, 600 });
    geom::Rectangle const clip {
        { 600, 0   },
        { 200, 600 }
    };

    auto const stretched = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, clip, mgl::Stretch { geom::Size { 300, 600 }, window });
    auto const settled = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip, std::nullopt);

    for (int i = 0; i < settled.nvertices; ++i)
    {
        EXPECT_FLOAT_EQ(stretched.vertices[i].position[0], settled.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].position[1], settled.vertices[i].position[1]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].texcoord[0], settled.vertices[i].texcoord[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].texcoord[1], settled.vertices[i].texcoord[1]) << "vertex " << i;
    }
}

TEST(TessellationHelpersTest, AShadowedClientFrozenAtItsMinimumStillMatchesTheUnstretchedCrop)
{
    // The same anti-pop guarantee for a CSD client, where the renderable is bigger than
    // and offset from the window. At scale 1.0 the shadow offset has to be carried
    // through untouched, so the frozen quad still lands where the renderable's own
    // rectangle would have put it.
    geom::Rectangle const window {
        { 600, 0   },
        { 300, 600 }
    };
    StubRenderable renderable({
                                  { 560, -40 },
                                  { 380, 680 }
    },
        { 380, 680 });
    geom::Rectangle const clip {
        { 600, 0   },
        { 200, 600 }
    };

    auto const stretched = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, clip, mgl::Stretch { geom::Size { 300, 600 }, window });
    auto const settled = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip, std::nullopt);

    for (int i = 0; i < settled.nvertices; ++i)
    {
        EXPECT_FLOAT_EQ(stretched.vertices[i].position[0], settled.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].position[1], settled.vertices[i].position[1]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].texcoord[0], settled.vertices[i].texcoord[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].texcoord[1], settled.vertices[i].texcoord[1]) << "vertex " << i;
    }
}

TEST(TessellationHelpersTest, AStretchWiderThanTheClipIsCroppedNotSquashed)
{
    // Shrinking past the minimum: the content stays 300 wide and slides under the clip's
    // right edge rather than being squeezed into 200.
    geom::Rectangle const window {
        { 600, 0   },
        { 300, 600 }
    };
    StubRenderable renderable(window, { 300, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 600, 0 }, { 200, 600 } }, mgl::Stretch { geom::Size { 300, 600 }, window });

    expect_quad(quad, 600.f, 0.f, 800.f, 600.f);
    expect_tex(quad, 0.f, 0.f, 2.f / 3.f, 1.f);
}

TEST(TessellationHelpersTest, AClientThatCannotShrinkStaysInsideItsTileForTheWholeAnimation)
{
    // The other half of the regression the recording caught: a window whose client refuses
    // to go below 300 while its tile animates down to 150. The clip is the tile - raw, not
    // run through the client's constraints - so every animated frame has to be cropped to
    // it. Handing the clip the constrained size instead left the window drawing at 300 for
    // the whole animation, overlapping its neighbour, until the final frame snapped it back.
    geom::Rectangle const window {
        { 600, 0   },
        { 300, 600 }
    };
    StubRenderable renderable(window, { 300, 600 });
    geom::Size const frozen { 300, 600 }; // the client's minimum; the stretch stops here

    for (int clip_width : { 300, 260, 220, 180, 150 })
    {
        geom::Rectangle const clip {
            { 600,        0   },
            { clip_width, 600 }
        };
        auto const quad = mgl::tessellate_renderable_into_rectangle(
            renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { frozen, window });

        auto const b = bounds_of(quad);
        EXPECT_FLOAT_EQ(b.left, 600.f) << "clip width " << clip_width;
        EXPECT_FLOAT_EQ(b.right, 600.f + static_cast<GLfloat>(clip_width)) << "clip width " << clip_width;
    }

    // And the last animated frame is pixel-for-pixel the settled frame that follows it,
    // so switching the stretch off at the end is invisible.
    geom::Rectangle const end {
        { 600, 0   },
        { 150, 600 }
    };
    auto const last_animated = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, end, mgl::Stretch { frozen, window });
    auto const settled = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, end, std::nullopt);

    for (int i = 0; i < settled.nvertices; ++i)
    {
        EXPECT_FLOAT_EQ(last_animated.vertices[i].position[0], settled.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(last_animated.vertices[i].position[1], settled.vertices[i].position[1]) << "vertex " << i;
        EXPECT_FLOAT_EQ(last_animated.vertices[i].texcoord[0], settled.vertices[i].texcoord[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(last_animated.vertices[i].texcoord[1], settled.vertices[i].texcoord[1]) << "vertex " << i;
    }
}

TEST(TessellationHelpersTest, AStretchNarrowerThanTheClipLeavesTheRestOfTheClipEmpty)
{
    // The grow-past-maximum case: a client capped at 400 stops growing while the clip
    // carries on to 800, so nothing is cropped and the background shows through the rest.
    geom::Rectangle const window {
        { 0,   0   },
        { 400, 600 }
    };
    StubRenderable renderable(window, { 400, 600 });

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 0, 0 }, { 800, 600 } }, mgl::Stretch { geom::Size { 400, 600 }, window });

    expect_quad(quad, 0.f, 0.f, 400.f, 600.f);
    expect_tex(quad, 0.f, 0.f, 1.f, 1.f);
}

TEST(TessellationHelpersTest, AFrozenStretchStaysAnchoredToAClipWhoseLeftEdgeIsMoving)
{
    // A tile shrinking from its left edge: the stretch size is frozen but the clip's
    // top-left keeps moving, and the quad has to follow it rather than the window.
    geom::Rectangle const window {
        { 600, 0   },
        { 300, 600 }
    };
    StubRenderable renderable(window, { 300, 600 });
    geom::Size const frozen { 300, 600 };

    auto const early = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 600, 0 }, { 250, 600 } }, mgl::Stretch { frozen, window });
    auto const late = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement {
                        0, 0
    },
        false, geom::Rectangle { { 700, 0 }, { 150, 600 } }, mgl::Stretch { frozen, window });

    expect_quad(early, 600.f, 0.f, 850.f, 600.f);
    expect_quad(late, 700.f, 0.f, 850.f, 600.f);
}

namespace
{
/// A client's fixed buffer inset, learned once the way the renderer learns it - from a
/// settled pairing of a committed buffer with the window size it was drawn for - and then
/// applied to whatever buffer happens to be on screen. Positive for a CSD drop shadow drawn
/// past the client's xdg window geometry, negative for a window whose buffer is only the
/// content inside a border.
struct Client
{
    geom::Displacement inset;

    static Client settled_at(geom::Size const& buffer, geom::Size const& window)
    {
        return {
            geom::Displacement {
                                buffer.width.as_int() - window.width.as_int(),
                                buffer.height.as_int() - window.height.as_int() }
        };
    }

    [[nodiscard]] geom::Rectangle showing(geom::Point const& top_left, geom::Size const& buffer) const
    {
        return mgl::committed_window_rect(top_left, buffer, inset);
    }
};
}

TEST(TessellationHelpersTest, ASettledUndecoratedClientHasNoInset)
{
    // Nothing decorated on either side: the buffer is the window it was given.
    EXPECT_EQ(
        Client::settled_at({ 780, 580 }, { 780, 580 }).inset,
        geom::Displacement(0, 0));
}

TEST(TessellationHelpersTest, ASettledCsdClientsInsetIsItsBufferOvershoot)
{
    // GTK draws its shadow into the buffer and declares the inner rectangle its window via
    // xdg window geometry, so mir hands the compositor a buffer bigger than the window -
    // here 26px of shadow each side and 40 below, on an 800x800 window.
    EXPECT_EQ(
        Client::settled_at({ 832, 846 }, { 800, 800 }).inset,
        geom::Displacement(32, 46));
}

TEST(TessellationHelpersTest, ASettledBorderedClientsInsetIsNegative)
{
    // The other direction, which the old measured band could not express at all: miracle
    // insets the content by border_size on every side, so the buffer is *smaller* than the
    // window it belongs to.
    EXPECT_EQ(
        Client::settled_at({ 780, 580 }, { 800, 600 }).inset,
        geom::Displacement(-20, -20));
}

TEST(TessellationHelpersTest, ACsdClientsWindowRectIsItsWindowSizeNotItsBuffer)
{
    // The regression gnome-clocks showed. Settled at an 800x800 window with a 832x846
    // buffer. The window rectangle has to come out as the window, 800x800 - not the buffer,
    // which would scale the whole window down by 800/832 for the length of every resize and
    // snap back at the end, with the shadow stretched over the tile in its place.
    auto const client = Client::settled_at({ 832, 846 }, { 800, 800 });
    EXPECT_EQ(
        client.showing({ 100, 100 }, { 832, 846 }),
        geom::Rectangle({ 100, 100 }, { 800, 800 }));
}

TEST(TessellationHelpersTest, ACsdClientsWindowRectTracksItsBufferNotItsLiveSize)
{
    // The same client mid-resize. The surface's live size has already jumped to the 400x400
    // target while the client still holds its pre-resize 832x846 buffer, so the window
    // rectangle has to stay the *old* 800x800 - that is what is on screen. Nothing here
    // consults the live size at all, which is exactly why it cannot be fooled by it.
    auto const client = Client::settled_at({ 832, 846 }, { 800, 800 });

    EXPECT_EQ(
        client.showing({ 100, 100 }, { 832, 846 }),
        geom::Rectangle({ 100, 100 }, { 800, 800 }));

    // And once it commits at the new size, the very same expression follows it there.
    EXPECT_EQ(
        client.showing({ 100, 100 }, { 432, 446 }),
        geom::Rectangle({ 100, 100 }, { 400, 400 }));
}

TEST(TessellationHelpersTest, AStaleInsetCannotProduceADegenerateWindowRect)
{
    // A client that dropped its shadow mid-animation leaves an inset wider than the buffer
    // that follows it. stretch_scale divides by this, so it is floored rather than allowed
    // to go to zero or invert.
    Client const client {
        geom::Displacement { 400, 400 }
    };
    EXPECT_EQ(client.showing({ 0, 0 }, { 40, 40 }).size, geom::Size(1, 1));
}

TEST(TessellationHelpersTest, AMarginedContentLandsOnTheDeflatedClipWhetherOrNotTheClientHasCaughtUp)
{
    // The frames the recording caught coming apart. miracle insets the content by
    // border_size on every side, so the renderable is the content rectangle, never the
    // window rectangle - and the client re-lays-out and commits its new buffer at some
    // unpredictable frame in the middle of the animation. Both sides of that commit have
    // to put the content on the clip deflated by the scaled margin: the window rectangle
    // the content implies is then the clip exactly, whatever size the buffer happens to
    // be. Before, the source size was guessed by nearest-match and a mid-animation commit
    // could pick an end that matched neither, leaving the content tens of pixels short of
    // the border on two sides.
    int const margin = 10;
    geom::Point const top_left { 100, 100 };

    // Animating 800x600 -> 400x300, with the clip halfway at 600x450.
    geom::Rectangle const clip {
        top_left, { 600, 450 }
    };

    struct Frame
    {
        char const* what;
        geom::Size content; // the size the client's committed buffer presents at
    };

    // The stale buffer is the pre-resize one (780x580 of content in an 800x600 window);
    // the caught-up one is the final 380x280 in a 400x300 window. Nothing in between is
    // ruled out either - a client may commit at any intermediate size - so the check is
    // written against the buffer size rather than against either end.
    // The live content size jumped to the 380x280 target on the animation's first frame
    // and stays there; only the committed buffer moves.
    // The client is bordered, so its buffer is the content inside the window: settled, it was
    // a 780x580 buffer in an 800x600 window.
    auto const client = Client::settled_at({ 780, 580 }, { 800, 600 });

    for (auto const& frame : {
             Frame { "stale",        { 780, 580 } },
             Frame { "intermediate", { 580, 430 } },
             Frame { "caught up",    { 380, 280 } }
    })
    {
        auto const natural = client.showing(top_left, frame.content);
        StubRenderable renderable(
            {
                { top_left.x.as_int() + margin, top_left.y.as_int() + margin },
                frame.content
        },
            frame.content);

        auto const quad = mgl::tessellate_renderable_into_rectangle(
            renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { clip.size, natural });

        GLfloat const scale = 600.f / static_cast<GLfloat>(natural.size.width.as_int());
        GLfloat const inset = static_cast<GLfloat>(margin) * scale;

        // The content is the clip deflated by the scaled margin - so re-inflating it by
        // that same margin gives the clip back, on every one of these frames. That is the
        // registration with the border that broke: the window rectangle the content
        // implies is the very rectangle draw_border is sized from.
        auto const b = bounds_of(quad);
        EXPECT_FLOAT_EQ(b.left - inset, 100.f) << frame.what;
        EXPECT_FLOAT_EQ(b.top - inset, 100.f) << frame.what;
        EXPECT_FLOAT_EQ(b.right + inset, 700.f) << frame.what;
        EXPECT_FLOAT_EQ(b.bottom + inset, 550.f) << frame.what;

        // Nothing is cropped: the content sits wholly inside the clip.
        expect_tex(quad, 0.f, 0.f, 1.f, 1.f);

        // The border itself is drawn at an unscaled border_size, so the content edge and
        // the border edge are apart by margin * |scale - 1| - bounded by the margin for
        // any scale up to 2, and zero by the time the animation settles. What the video
        // showed was that gap running to tens of pixels, i.e. unbounded by the margin.
        EXPECT_LE(std::abs(b.left - (100.f + static_cast<GLfloat>(margin))),
            static_cast<GLfloat>(margin))
            << frame.what;
        EXPECT_LE(std::abs(b.bottom - (550.f - static_cast<GLfloat>(margin))),
            static_cast<GLfloat>(margin))
            << frame.what;
    }
}

TEST(TessellationHelpersTest, AShadowedClientIsDrawnAtItsWindowSizeForTheWholeResize)
{
    // gnome-clocks, end to end. Settled at an 800x800 window with a 10px border, so
    // 780x780 of content, and a GTK drop shadow of 26px on three sides and 40 below - a
    // 832x846 buffer whose top-left is 26px up and left of the content. The tile is
    // animating down to 400x400 and is halfway, at 600x600.
    int const margin = 10;
    geom::Displacement const shadow { 52, 66 };
    geom::Point const top_left { 100, 100 };
    geom::Rectangle const clip {
        top_left, { 600, 600 }
    };

    StubRenderable renderable({
                                  { 84,  84  },
                                  { 832, 846 }
    },
        { 832, 846 });
    auto const natural = Client::settled_at({ 832, 846 }, { 800, 800 }).showing(top_left, { 832, 846 });

    // The buffer is measured as the 800x800 window it was drawn for, not as the 852x866
    // that buffer-plus-margins would make it. Getting that wrong scaled the whole window
    // by 800/852 for the length of every resize, which is the client visibly shrinking.
    EXPECT_EQ(natural, geom::Rectangle(top_left, { 800, 800 }));

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { clip.size, natural });

    // At scale 0.75 the shadow spills past the clip on every side and is cropped away, so
    // the quad is the clip exactly...
    expect_quad(quad, 100.f, 100.f, 700.f, 700.f);

    // ...and the content inside it lands on the clip deflated by the scaled margin, which
    // is where the border puts it.
    GLfloat const scale = 0.75f;
    GLfloat const content_left = 100.f - 16.f * scale + 26.f * scale;
    EXPECT_FLOAT_EQ(content_left, 100.f + static_cast<GLfloat>(margin) * scale);

    // The surviving fraction of the buffer is the part of the shadow that is inside the
    // window rectangle - the margin band the border is drawn over - and no more.
    GLfloat const w = 832.f * scale, h = 846.f * scale;
    expect_tex(quad, 12.f / w, 12.f / h, 612.f / w, 612.f / h);
}

TEST(TessellationHelpersTest, AShadowedClientsShadowIsNeverStretchedOverItsTile)
{
    // The reported artifact, isolated. A CSD client with a 40px shadow on every side grows
    // 800 -> 1200 and the clip is a quarter of the way, at 900.
    geom::Point const top_left { 100, 100 };
    geom::Rectangle const clip {
        top_left, { 900, 600 }
    };

    // The buffer sits 40px up and left of the window and overshoots it by 80 on each axis.
    StubRenderable renderable({
                                  { 60,  60  },
                                  { 880, 680 }
    },
        { 880, 680 });

    auto const client = Client::settled_at({ 880, 680 }, { 800, 600 });
    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip,
        mgl::Stretch { clip.size, client.showing(top_left, { 880, 680 }) });

    // At scale 900/800 the shadow spills past the clip on every side and is cropped away, so
    // the quad is the clip exactly - no band anywhere, least of all on the right.
    expect_quad(quad, 100.f, 100.f, 1000.f, 700.f);

    // The same frame with the inset unaccounted for, which is what the old measured band
    // gave until some settled frame happened to land first. The buffer then measures 880
    // rather than the 800 window it belongs to, so the whole thing is scaled by 900/880
    // instead of 900/800 - and the shadow, rather than the content, is what ends up filling
    // the tile.
    auto const unlearned = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip,
        mgl::Stretch { clip.size, mgl::committed_window_rect(top_left, { 880, 680 }, {}) });

    // Nothing reaches the clip's right edge: the quad stops ~41px short of it and the tile
    // background shows through the rest. That gap is the artifact.
    auto const b = bounds_of(unlearned);
    EXPECT_LT(b.right, 1000.f);
    EXPECT_NEAR(1000.f - b.right, 40.9f, 0.5f);

    // And it is a gap rather than a crop: the source is sampled all the way to its right
    // edge, so there is simply no more quad to draw there.
    EXPECT_FLOAT_EQ(tex_of(unlearned).right, 1.f);
}

TEST(TessellationHelpersTest, AStretchIsAtRestOnTheFrameTheAnimationStarts)
{
    // t=0: the clip is still the window the client is settled at, so the stretch must be the
    // identity - the same pixels in the same place as the un-animated frame before it.
    geom::Point const top_left { 100, 100 };
    geom::Rectangle const window {
        top_left, { 800, 600 }
    };

    StubRenderable renderable({
                                  { 60,  60  },
                                  { 880, 680 }
    },
        { 880, 680 });

    auto const client = Client::settled_at({ 880, 680 }, { 800, 600 });
    auto const stretched = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, window,
        mgl::Stretch { window.size, client.showing(top_left, { 880, 680 }) });

    auto const unstretched = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, window);

    for (int i = 0; i < stretched.nvertices; ++i)
    {
        EXPECT_FLOAT_EQ(stretched.vertices[i].position[0], unstretched.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].position[1], unstretched.vertices[i].position[1]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].texcoord[0], unstretched.vertices[i].texcoord[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(stretched.vertices[i].texcoord[1], unstretched.vertices[i].texcoord[1]) << "vertex " << i;
    }
}

TEST(TessellationHelpersTest, AnExactNaturalSizeKeepsAStaleBufferFillingTheClip)
{
    // The frame that used to flash, now with the natural size derived rather than guessed:
    // an 800x600 buffer on an undecorated window already told to be 200 wide, with the clip
    // still near the start of the animation. The buffer's own size *is* the window size it
    // corresponds to, so the content still fills the clip - the same thing the frame before
    // it drew - rather than being magnified by 800/200.
    geom::Rectangle const old_window {
        { 0,   0   },
        { 800, 600 }
    };
    StubRenderable renderable(old_window, { 800, 600 });
    geom::Rectangle const clip {
        { 0,   0   },
        { 780, 600 }
    };

    auto const quad = mgl::tessellate_renderable_into_rectangle(
        renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { clip.size, Client::settled_at(old_window.size, old_window.size).showing(old_window.top_left, old_window.size) });

    expect_quad(quad, 0.f, 0.f, 780.f, 600.f);
    expect_tex(quad, 0.f, 0.f, 1.f, 1.f);
}

TEST(TessellationHelpersTest, TheQuadDoesNotMoveWhenTheClientCatchesUp)
{
    // The stretch is only worth anything if the drawn rectangle stays put across the frame
    // the client commits its new buffer on - that commit is exactly where the pop used to
    // be. Before it, the live layer is a stale buffer measured against its own natural
    // rectangle; after it, a smaller buffer scaled up by more. Both have to land in the
    // same place.
    //
    // A window shrinking 800 -> 400, with the animated clip - already clamped to what the
    // client can reach - passing through 600.
    geom::Rectangle const before {
        { 0,   0   },
        { 800, 600 }
    };
    geom::Rectangle const after {
        { 0,   0   },
        { 400, 600 }
    };
    geom::Rectangle const clip {
        { 0,   0   },
        { 600, 600 }
    };
    geom::Size const stretch { 600, 600 };

    // While the client still has not committed at its new size, the live layer is drawing
    // the pre-resize buffer, measured against the pre-resize natural rectangle.
    StubRenderable stale(before, before.size);
    auto const live_stale = mgl::tessellate_renderable_into_rectangle(
        stale, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { stretch, before });

    // Once it has caught up, the live layer is a smaller buffer scaled up by more, which
    // has to land on exactly the same rectangle.
    StubRenderable caught_up(after, after.size);
    auto const live_caught_up = mgl::tessellate_renderable_into_rectangle(
        caught_up, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { stretch, after });

    for (int i = 0; i < live_stale.nvertices; ++i)
    {
        EXPECT_FLOAT_EQ(live_caught_up.vertices[i].position[0], live_stale.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(live_caught_up.vertices[i].position[1], live_stale.vertices[i].position[1]) << "vertex " << i;
    }
}
