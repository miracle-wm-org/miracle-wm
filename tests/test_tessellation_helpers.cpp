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
/// The window rectangle a committed buffer corresponds to, derived the way the renderer
/// derives it. \p content is the live content size, which during a resize has already
/// moved to the target while \p buffer_size is still whatever the client last committed.
geom::Rectangle natural_of(
    geom::Point const& window_top_left,
    geom::Size const& buffer_size,
    geom::Size const& content,
    int margin,
    geom::Displacement const& shadow = {})
{
    geom::Size const window { content.width.as_int() + 2 * margin, content.height.as_int() + 2 * margin };
    return mgl::natural_window_rect(window_top_left, buffer_size, window, content, shadow);
}
}

TEST(TessellationHelpersTest, ASettledUndecoratedClientHasNoShadowBand)
{
    // Everything server-side decorated: the buffer is the content it was given.
    EXPECT_EQ(mgl::shadow_band(geom::Size { 780, 580 }, geom::Size { 780, 580 }), geom::Displacement(0, 0));
}

TEST(TessellationHelpersTest, ASettledCsdClientsShadowBandIsTheBufferOvershoot)
{
    // GTK draws its shadow into the buffer and declares the inner rectangle its window
    // via xdg window geometry, so mir hands the compositor a buffer bigger than the
    // content size it asked for - here 26px of shadow each side and 40 below.
    EXPECT_EQ(
        mgl::shadow_band(geom::Size { 832, 846 }, geom::Size { 780, 780 }),
        geom::Displacement(52, 66));
}

TEST(TessellationHelpersTest, AClientThatHasNotCaughtUpNeverYieldsANegativeShadowBand)
{
    // Mid-grow: the content size has already jumped to the target and the buffer has not.
    // That is not a negative shadow, and treating it as one would inflate the window
    // rectangle - the magnification this whole path exists to avoid.
    EXPECT_EQ(mgl::shadow_band(geom::Size { 400, 300 }, geom::Size { 800, 600 }), geom::Displacement(0, 0));
}

TEST(TessellationHelpersTest, ACsdClientsNaturalSizeIsItsWindowSizeNotItsBuffer)
{
    // The regression gnome-clocks showed. Settled at a 780x780 content size inside an
    // 800x800 window (10px border), with a 52x66 shadow band making the buffer 832x846.
    // The natural rectangle has to come out as the window, 800x800 - not the buffer plus
    // margins, 852x866, which would scale the whole window down by 800/852 for the length
    // of every resize and snap back at the end.
    auto const natural = natural_of({ 100, 100 }, { 832, 846 }, { 780, 780 }, 10, geom::Displacement { 52, 66 });
    EXPECT_EQ(natural, geom::Rectangle({ 100, 100 }, { 800, 800 }));

    // And the shadow band it was built from is the one a settled frame measures, so the
    // renderer's remembered value and this are the same number.
    EXPECT_EQ(mgl::shadow_band(geom::Size { 832, 846 }, geom::Size { 780, 780 }), geom::Displacement(52, 66));
}

TEST(TessellationHelpersTest, ACsdClientsNaturalSizeTracksItsBufferNotItsLiveContentSize)
{
    // Mid-resize, the same client: the live content size has jumped to the 380x380 target
    // while the client still holds its pre-resize 832x846 buffer. The natural rectangle
    // has to stay the *old* 800x800 window, because that is what is on screen. The shadow
    // band is the one remembered from before the resize.
    geom::Displacement const shadow { 52, 66 };
    EXPECT_EQ(
        natural_of({ 100, 100 }, { 832, 846 }, { 380, 380 }, 10, shadow),
        geom::Rectangle({ 100, 100 }, { 800, 800 }));

    // And once it commits at the new size, the very same expression follows it there.
    EXPECT_EQ(
        natural_of({ 100, 100 }, { 432, 446 }, { 380, 380 }, 10, shadow),
        geom::Rectangle({ 100, 100 }, { 400, 400 }));
}

TEST(TessellationHelpersTest, AStaleShadowBandCannotProduceADegenerateNaturalRect)
{
    // A client that dropped its shadow mid-animation leaves a band wider than the buffer
    // that follows it. stretch_scale divides by this, so it is floored rather than
    // allowed to go to zero or invert.
    auto const natural = natural_of({ 0, 0 }, { 40, 40 }, { 40, 40 }, 0, geom::Displacement { 400, 400 });
    EXPECT_EQ(natural.size, geom::Size(1, 1));
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
    geom::Size const live_content { 380, 280 };

    for (auto const& frame : {
             Frame { "stale",        { 780, 580 } },
             Frame { "intermediate", { 580, 430 } },
             Frame { "caught up",    { 380, 280 } }
    })
    {
        auto const natural = natural_of(top_left, frame.content, live_content, margin);
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
    auto const natural = natural_of(top_left, { 832, 846 }, { 380, 380 }, margin, shadow);

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
        renderable, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { clip.size, natural_of(old_window.top_left, old_window.size, old_window.size, 0) });

    expect_quad(quad, 0.f, 0.f, 780.f, 600.f);
    expect_tex(quad, 0.f, 0.f, 1.f, 1.f);
}

TEST(TessellationHelpersTest, TheRawOverloadReproducesTheRenderableOverload)
{
    // The two share one body so the layers of a cross-fade cannot drift apart. This pins
    // the split itself: whatever the renderable overload reads off a renderable, the raw
    // one must produce from the same three values spelled out.
    geom::Rectangle const window {
        { 10,  20  },
        { 800, 600 }
    };
    geom::RectangleD const src {
        { 0,   0   },
        { 512, 512 }
    };
    StubRenderable renderable(window, { 1024, 1024 }, src);

    struct Case
    {
        std::optional<geom::Rectangle> clip;
        std::optional<mgl::Stretch> stretch;
        bool flipped;
    };

    std::vector<Case> const cases {
        { std::nullopt,                                 std::nullopt,                                     false },
        { geom::Rectangle { { 10, 20 }, { 400, 600 } }, std::nullopt,                                     true  },
        { geom::Rectangle { { 60, 20 }, { 300, 200 } }, mgl::Stretch { geom::Size { 300, 200 }, window }, false },
        { geom::Rectangle { { 60, 20 }, { 900, 700 } }, mgl::Stretch { geom::Size { 900, 700 }, window }, true  },
    };

    for (auto const& c : cases)
    {
        auto const from_renderable = mgl::tessellate_renderable_into_rectangle(
            renderable, geom::Displacement { 0, 0 }, c.flipped, c.clip, c.stretch);
        auto const raw = mgl::tessellate_into_rectangle(
            window, geom::Size { 1024, 1024 }, src,
            geom::Displacement { 0, 0 }, c.flipped, c.clip, c.stretch);

        for (int i = 0; i < from_renderable.nvertices; ++i)
        {
            EXPECT_FLOAT_EQ(raw.vertices[i].position[0], from_renderable.vertices[i].position[0]) << "vertex " << i;
            EXPECT_FLOAT_EQ(raw.vertices[i].position[1], from_renderable.vertices[i].position[1]) << "vertex " << i;
            EXPECT_FLOAT_EQ(raw.vertices[i].texcoord[0], from_renderable.vertices[i].texcoord[0]) << "vertex " << i;
            EXPECT_FLOAT_EQ(raw.vertices[i].texcoord[1], from_renderable.vertices[i].texcoord[1]) << "vertex " << i;
        }
    }
}

TEST(TessellationHelpersTest, AGhostQuadRegistersWithTheLiveQuadOnBothSidesOfTheClientCatchingUp)
{
    // The cross-fade only works if the two layers occupy the same rectangle: a ghost that
    // slid against the live surface would be worse than the pop it is there to hide. The
    // ghost is built from values captured before the resize and has no live renderable to
    // ask, so this is the check that the capture carries enough to land in the same place.
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

    // The ghost's provenance is exact by construction: the natural rectangle was captured
    // alongside the buffer, so it needs no guessing at all.
    auto const ghost = mgl::tessellate_into_rectangle(
        before, before.size, geom::RectangleD {
                                 { 0,   0   },
                                 { 800, 600 }
    },
        geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { stretch, before });

    // While the client still has not committed at its new size, the live layer is drawing
    // the very same buffer measured the very same way, so the two are identical.
    StubRenderable stale(before, before.size);
    auto const live_stale = mgl::tessellate_renderable_into_rectangle(
        stale, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { stretch, before });

    // Once it has caught up, the live layer is a smaller buffer scaled up by more, which
    // has to land on exactly the same rectangle - that is the frame the ghost is hiding.
    StubRenderable caught_up(after, after.size);
    auto const live_caught_up = mgl::tessellate_renderable_into_rectangle(
        caught_up, geom::Displacement { 0, 0 }, false, clip, mgl::Stretch { stretch, after });

    for (int i = 0; i < ghost.nvertices; ++i)
    {
        EXPECT_FLOAT_EQ(live_stale.vertices[i].position[0], ghost.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(live_stale.vertices[i].position[1], ghost.vertices[i].position[1]) << "vertex " << i;
        EXPECT_FLOAT_EQ(live_caught_up.vertices[i].position[0], ghost.vertices[i].position[0]) << "vertex " << i;
        EXPECT_FLOAT_EQ(live_caught_up.vertices[i].position[1], ghost.vertices[i].position[1]) << "vertex " << i;
    }
}
