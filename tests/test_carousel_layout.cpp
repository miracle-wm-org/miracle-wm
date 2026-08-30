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

#include "carousel_layout.h"

#include <gtest/gtest.h>

using namespace miracle;
namespace geom = mir::geometry;

namespace
{
geom::Rectangle const BOUNDS {
    { 0,    0    },
    { 1920, 1080 }
};

/// A window whose size varies with \p index, so that placements can be traced
/// back to the window they came from.
geom::Rectangle window_of(size_t index)
{
    return geom::Rectangle {
        { 0,                                  0                                  },
        { 400 + 30 * static_cast<int>(index), 300 + 10 * static_cast<int>(index) }
    };
}

std::vector<geom::Rectangle> windows(size_t count)
{
    std::vector<geom::Rectangle> result;
    for (size_t i = 0; i < count; i++)
        result.push_back(window_of(i));
    return result;
}

std::vector<geom::Rectangle> uniform_windows(size_t count)
{
    return std::vector<geom::Rectangle>(count, geom::Rectangle {
                                                   { 0,   0   },
                                                   { 800, 600 }
    });
}

bool overlaps(carousel_layout::Placement const& a, carousel_layout::Placement const& b)
{
    return a.x < b.x + b.width
        && b.x < a.x + a.width
        && a.y < b.y + b.height
        && b.y < a.y + a.height;
}

float center_x(carousel_layout::Placement const& placement)
{
    return placement.x + placement.width / 2.f;
}

float aspect(carousel_layout::Placement const& placement)
{
    return placement.width / placement.height;
}
}

TEST(CarouselLayout, EmptyInputProducesNoPlacements)
{
    EXPECT_TRUE(carousel_layout::compute(BOUNDS, {}, 0.f).empty());
}

TEST(CarouselLayout, ProducesOnePlacementPerWindowInInputOrder)
{
    auto const input = windows(6);
    auto const result = carousel_layout::compute(BOUNDS, input, 0.f);

    ASSERT_EQ(input.size(), result.size());
    for (size_t i = 0; i < input.size(); i++)
    {
        auto const expected = static_cast<float>(input[i].size.width.as_value())
            / static_cast<float>(input[i].size.height.as_value());
        EXPECT_NEAR(expected, aspect(result[i]), 0.001f) << "at index " << i;
    }
}

TEST(CarouselLayout, PreservesAspectRatioOfEveryWindow)
{
    std::vector<geom::Rectangle> const input {
        { { 0, 0 }, { 1920, 1080 } },
        { { 0, 0 }, { 300, 900 }   },
        { { 0, 0 }, { 640, 480 }   },
        { { 0, 0 }, { 1000, 100 }  },
    };

    auto const result = carousel_layout::compute(BOUNDS, input, 1.f);
    for (size_t i = 0; i < input.size(); i++)
    {
        auto const expected = static_cast<float>(input[i].size.width.as_value())
            / static_cast<float>(input[i].size.height.as_value());
        EXPECT_NEAR(expected, aspect(result[i]), 0.001f) << "at index " << i;
    }
}

TEST(CarouselLayout, NeverScalesAWindowAboveMaxScale)
{
    // Small enough that its slot box could hold it several times over.
    std::vector<geom::Rectangle> const input(3, geom::Rectangle {
                                                    { 0,  0  },
                                                    { 40, 30 }
    });

    auto const result = carousel_layout::compute(BOUNDS, input, 0.f);
    for (auto const& placement : result)
    {
        EXPECT_LE(placement.width, 40.f);
        EXPECT_LE(placement.height, 30.f);
    }
}

TEST(CarouselLayout, PlacementsNeverOverlapAtAnyPosition)
{
    for (size_t count = 1; count <= 12; count++)
    {
        auto const input = windows(count);
        for (int tenths = 0; tenths <= static_cast<int>(count) * 10; tenths++)
        {
            auto const position = static_cast<float>(tenths) / 10.f;
            auto const result = carousel_layout::compute(BOUNDS, input, position);

            for (size_t i = 0; i < result.size(); i++)
            {
                for (size_t j = i + 1; j < result.size(); j++)
                {
                    EXPECT_FALSE(overlaps(result[i], result[j]))
                        << "windows " << i << " and " << j << " of " << count
                        << " overlap at position " << position;
                }
            }
        }
    }
}

TEST(CarouselLayout, TheWindowAtThePositionIsCenteredAndFullyOpaque)
{
    auto const input = windows(7);
    for (size_t position = 0; position < input.size(); position++)
    {
        auto const result = carousel_layout::compute(BOUNDS, input, static_cast<float>(position));

        EXPECT_NEAR(960.f, center_x(result[position]), 0.001f);
        EXPECT_NEAR(540.f, result[position].y + result[position].height / 2.f, 0.001f);
        EXPECT_NEAR(1.f, result[position].opacity, 0.001f);
    }
}

TEST(CarouselLayout, TheWindowAtThePositionIsTheLargest)
{
    auto const input = uniform_windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 2.f);

    for (size_t i = 0; i < result.size(); i++)
    {
        if (i == 2)
            continue;
        EXPECT_LT(result[i].width, result[2].width) << "at index " << i;
    }
}

TEST(CarouselLayout, WindowsAStepOrMoreFromTheCenterAreDimmed)
{
    carousel_layout::Options const options;
    auto const input = windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 2.f);

    for (size_t i = 0; i < result.size(); i++)
    {
        if (i == 2)
            continue;
        EXPECT_NEAR(options.dim, result[i].opacity, 0.001f) << "at index " << i;
    }
}

TEST(CarouselLayout, DimmingIsInterpolatedWhileScrolling)
{
    carousel_layout::Options const options;
    auto const input = windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 0.5f);

    auto const expected = options.dim + (1.f - options.dim) * 0.5f;
    EXPECT_NEAR(expected, result[0].opacity, 0.001f);
    EXPECT_NEAR(expected, result[1].opacity, 0.001f);
}

TEST(CarouselLayout, TheStripHasEndsRatherThanWrapping)
{
    auto const input = windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 0.f);

    // With the first window centered there is nothing to its left: the rest of
    // the strip runs off to the right, in order.
    for (size_t i = 1; i < result.size(); i++)
        EXPECT_GT(center_x(result[i]), center_x(result[i - 1])) << "at index " << i;
}

TEST(CarouselLayout, WindowsBeforeThePositionSitToItsLeft)
{
    auto const input = windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 3.f);

    EXPECT_LT(center_x(result[0]), center_x(result[2]));
    EXPECT_LT(center_x(result[2]), center_x(result[3]));
    EXPECT_GT(center_x(result[4]), center_x(result[3]));
}

TEST(CarouselLayout, ASingleWindowSitsInTheMiddle)
{
    auto const result = carousel_layout::compute(BOUNDS, uniform_windows(1), 0.f);

    ASSERT_EQ(1u, result.size());
    EXPECT_NEAR(960.f, center_x(result[0]), 0.001f);
    EXPECT_NEAR(1.f, result[0].opacity, 0.001f);
}

TEST(CarouselLayout, TheOutermostWindowsHangOffTheEdgesOfTheBounds)
{
    auto const input = uniform_windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 2.f);

    // The centered window is wholly on screen...
    EXPECT_GE(result[2].x, 0.f);
    EXPECT_LE(result[2].x + result[2].width, 1920.f);

    // ...the pair beside it straddles an edge...
    EXPECT_LT(result[1].x, 0.f);
    EXPECT_GT(result[1].x + result[1].width, 0.f);
    EXPECT_LT(result[3].x, 1920.f);
    EXPECT_GT(result[3].x + result[3].width, 1920.f);

    // ...and the pair beyond that is off screen entirely, for the renderer to
    // cull.
    EXPECT_LE(result[0].x + result[0].width, 0.f);
    EXPECT_GE(result[4].x, 1920.f);
}

TEST(CarouselLayout, NeighbouringWindowsSitExactlyTheGapApart)
{
    carousel_layout::Options const options;
    auto const input = windows(5);

    // Spacing follows the sizes the windows are actually drawn at, so it holds
    // mid-scroll too, when no window is at its full slot size.
    for (auto const position : { 0.f, 2.f, 2.5f, 4.f })
    {
        auto const result = carousel_layout::compute(BOUNDS, input, position);
        for (size_t i = 1; i < result.size(); i++)
        {
            auto const distance = result[i].x - (result[i - 1].x + result[i - 1].width);
            EXPECT_NEAR(options.gap, distance, 0.001f)
                << "between " << i - 1 << " and " << i << " at position " << position;
        }
    }
}

TEST(CarouselLayout, ASpacingBelowOneOverlapsNeighbours)
{
    carousel_layout::Options options;
    options.spacing = 0.5f;
    options.gap = 0.f;

    auto const result = carousel_layout::compute(BOUNDS, uniform_windows(3), 1.f, options);
    EXPECT_TRUE(overlaps(result[0], result[1]));
    EXPECT_TRUE(overlaps(result[1], result[2]));
}

TEST(CarouselLayout, IsDeterministic)
{
    auto const input = windows(6);
    auto const first = carousel_layout::compute(BOUNDS, input, 2.5f);
    auto const second = carousel_layout::compute(BOUNDS, input, 2.5f);

    ASSERT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); i++)
    {
        EXPECT_EQ(first[i].x, second[i].x);
        EXPECT_EQ(first[i].y, second[i].y);
        EXPECT_EQ(first[i].width, second[i].width);
        EXPECT_EQ(first[i].height, second[i].height);
        EXPECT_EQ(first[i].opacity, second[i].opacity);
    }
}

TEST(CarouselLayout, LerpMovesFromOnePlacementToTheOther)
{
    carousel_layout::Placement const from { .x = 0.f, .y = 0.f, .width = 100.f, .height = 50.f, .opacity = 0.f };
    carousel_layout::Placement const to { .x = 100.f, .y = 200.f, .width = 200.f, .height = 150.f, .opacity = 1.f };

    auto const half = carousel_layout::lerp(from, to, 0.5f);
    EXPECT_NEAR(50.f, half.x, 0.001f);
    EXPECT_NEAR(100.f, half.y, 0.001f);
    EXPECT_NEAR(150.f, half.width, 0.001f);
    EXPECT_NEAR(100.f, half.height, 0.001f);
    EXPECT_NEAR(0.5f, half.opacity, 0.001f);
}

TEST(CarouselLayout, ContainsHitTestsThePlacementRectangle)
{
    carousel_layout::Placement const placement { .x = 10.f, .y = 20.f, .width = 100.f, .height = 50.f };

    EXPECT_TRUE(carousel_layout::contains(placement, 10.f, 20.f));
    EXPECT_TRUE(carousel_layout::contains(placement, 109.f, 69.f));
    EXPECT_FALSE(carousel_layout::contains(placement, 110.f, 70.f));
    EXPECT_FALSE(carousel_layout::contains(placement, 9.f, 40.f));
}

// -----------------------------------------------------------------------------
// fit
// -----------------------------------------------------------------------------

TEST(CarouselLayout, FitIsTheIdentityWhenTheTileMatchesTheSource)
{
    carousel_layout::Placement const tile {
        .x = 0.f, .y = 0.f, .width = 1920.f, .height = 1080.f, .opacity = 1.f
    };
    geom::Rectangle const window {
        { 100, 200 },
        { 400, 300 }
    };

    auto const fitted = carousel_layout::fit(BOUNDS, tile, window);
    EXPECT_FLOAT_EQ(fitted.x, 100.f);
    EXPECT_FLOAT_EQ(fitted.y, 200.f);
    EXPECT_FLOAT_EQ(fitted.width, 400.f);
    EXPECT_FLOAT_EQ(fitted.height, 300.f);
}

TEST(CarouselLayout, FitScalesAndOffsetsIntoAHalfSizeTile)
{
    carousel_layout::Placement const tile {
        .x = 500.f, .y = 40.f, .width = 960.f, .height = 540.f, .opacity = 1.f
    };
    geom::Rectangle const window {
        { 100, 200 },
        { 400, 300 }
    };

    auto const fitted = carousel_layout::fit(BOUNDS, tile, window);
    EXPECT_FLOAT_EQ(fitted.x, 500.f + 50.f);
    EXPECT_FLOAT_EQ(fitted.y, 40.f + 100.f);
    EXPECT_FLOAT_EQ(fitted.width, 200.f);
    EXPECT_FLOAT_EQ(fitted.height, 150.f);
}

TEST(CarouselLayout, FitIsRelativeToTheSourceOrigin)
{
    geom::Rectangle const source {
        { 1920, 0    },
        { 1920, 1080 }
    };
    carousel_layout::Placement const tile {
        .x = 0.f, .y = 0.f, .width = 960.f, .height = 540.f, .opacity = 1.f
    };
    geom::Rectangle const window {
        { 1920 + 200, 100 },
        { 400,        300 }
    };

    auto const fitted = carousel_layout::fit(source, tile, window);
    EXPECT_FLOAT_EQ(fitted.x, 100.f);
    EXPECT_FLOAT_EQ(fitted.y, 50.f);
}

TEST(CarouselLayout, FitInheritsTheTileOpacity)
{
    carousel_layout::Placement const tile {
        .x = 0.f, .y = 0.f, .width = 960.f, .height = 540.f, .opacity = 0.55f
    };

    auto const fitted = carousel_layout::fit(BOUNDS, tile, window_of(0));
    EXPECT_FLOAT_EQ(fitted.opacity, 0.55f);
}

// -----------------------------------------------------------------------------
// workspace_options
// -----------------------------------------------------------------------------

namespace
{
/// The workspace strip lays out one tile per workspace, each a picture of the
/// whole output.
std::vector<carousel_layout::Placement> workspace_strip(size_t count, float position)
{
    std::vector<geom::Rectangle> const sources(count, BOUNDS);
    return carousel_layout::compute(BOUNDS, sources, position, carousel_layout::workspace_options);
}
}

TEST(CarouselLayout, WorkspaceStripKeepsTheCenteredTileWhollyOnScreen)
{
    for (size_t count : { 1u, 2u, 5u })
    {
        for (size_t position = 0; position < count; position++)
        {
            auto const tiles = workspace_strip(count, static_cast<float>(position));
            auto const& centered = tiles[position];
            EXPECT_GE(centered.x, 0.f) << "count " << count << " position " << position;
            EXPECT_LE(centered.x + centered.width, 1920.f) << "count " << count << " position " << position;
            EXPECT_GE(centered.y, 0.f);
            EXPECT_LE(centered.y + centered.height, 1080.f);
        }
    }
}

TEST(CarouselLayout, WorkspaceStripLeavesTheNeighboursStraddlingTheEdges)
{
    auto const tiles = workspace_strip(5, 2.f);

    // The tile on either side of the centered one is partly on screen and partly
    // off it, which is what advertises that there is more desktop over there.
    for (size_t neighbour : { 1u, 3u })
    {
        EXPECT_LT(tiles[neighbour].x, 1920.f);
        EXPECT_GT(tiles[neighbour].x + tiles[neighbour].width, 0.f);
        bool const straddles = tiles[neighbour].x < 0.f
            || tiles[neighbour].x + tiles[neighbour].width > 1920.f;
        EXPECT_TRUE(straddles) << "neighbour " << neighbour;
    }
}

TEST(CarouselLayout, WorkspaceStripDimsEverythingButTheCenteredTile)
{
    auto const tiles = workspace_strip(3, 1.f);
    EXPECT_FLOAT_EQ(tiles[1].opacity, 1.f);
    EXPECT_FLOAT_EQ(tiles[0].opacity, carousel_layout::workspace_options.dim);
    EXPECT_FLOAT_EQ(tiles[2].opacity, carousel_layout::workspace_options.dim);
}

TEST(CarouselLayout, WorkspaceStripTilesDoNotOverlap)
{
    auto const tiles = workspace_strip(5, 2.f);
    for (size_t i = 0; i + 1 < tiles.size(); i++)
        EXPECT_FALSE(overlaps(tiles[i], tiles[i + 1])) << "tiles " << i << " and " << i + 1;
}
