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

#include "spread_layout.h"

#include <gtest/gtest.h>

using namespace miracle;
namespace geom = mir::geometry;

namespace
{
geom::Rectangle const BOUNDS {
    { 0,    0    },
    { 1920, 1080 }
};

bool within(geom::Rectangle const& inner, geom::Rectangle const& outer)
{
    return inner.top_left.x >= outer.top_left.x
        && inner.top_left.y >= outer.top_left.y
        && inner.right() <= outer.right()
        && inner.bottom() <= outer.bottom();
}
}

TEST(SpreadLayoutTest, EmptyInputYieldsEmptyOutput)
{
    EXPECT_TRUE(spread_layout::compute(BOUNDS, {}).empty());
}

TEST(SpreadLayoutTest, SingleWindowStaysInBounds)
{
    auto const result = spread_layout::compute(BOUNDS, {
                                                           geom::Rectangle { { 700, 400 }, { 500, 300 } }
    });
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(within(result[0], BOUNDS));
}

TEST(SpreadLayoutTest, ReturnsOneRectanglePerInputInOrder)
{
    std::vector<geom::Rectangle> const windows {
        geom::Rectangle { { 0, 0 },     { 960, 1080 } },
        geom::Rectangle { { 960, 0 },   { 960, 540 }  },
        geom::Rectangle { { 960, 540 }, { 960, 540 }  },
    };
    auto const result = spread_layout::compute(BOUNDS, windows);
    ASSERT_EQ(result.size(), windows.size());
}

TEST(SpreadLayoutTest, OverlappingWindowsAreSeparated)
{
    // Five identical windows stacked at the same position.
    std::vector<geom::Rectangle> const windows(5, geom::Rectangle {
                                                      { 500, 300 },
                                                      { 600, 400 }
    });
    int const gap = 16;
    auto const result = spread_layout::compute(BOUNDS, windows, gap);
    ASSERT_EQ(result.size(), windows.size());

    for (size_t i = 0; i < result.size(); ++i)
    {
        EXPECT_TRUE(within(result[i], BOUNDS)) << "window " << i << " escaped the bounds";
        for (size_t j = i + 1; j < result.size(); ++j)
            EXPECT_FALSE(result[i].overlaps(result[j])) << "windows " << i << " and " << j << " overlap";
    }
}

TEST(SpreadLayoutTest, IsDeterministic)
{
    std::vector<geom::Rectangle> const windows {
        geom::Rectangle { { 100, 100 }, { 800, 600 } },
        geom::Rectangle { { 200, 150 }, { 640, 480 } },
        geom::Rectangle { { 900, 500 }, { 700, 500 } },
    };
    auto const a = spread_layout::compute(BOUNDS, windows);
    auto const b = spread_layout::compute(BOUNDS, windows);
    EXPECT_EQ(a, b);
}

TEST(SpreadLayoutTest, ManyWindowsOnSmallBoundsTerminatesAndStaysInBounds)
{
    geom::Rectangle const small_bounds {
        { 0,   0   },
        { 800, 600 }
    };
    std::vector<geom::Rectangle> const windows(20, geom::Rectangle {
                                                       { 100, 100 },
                                                       { 400, 300 }
    });
    auto const result = spread_layout::compute(small_bounds, windows);
    ASSERT_EQ(result.size(), windows.size());
    for (size_t i = 0; i < result.size(); ++i)
        EXPECT_TRUE(within(result[i], small_bounds)) << "window " << i << " escaped the bounds";
}

TEST(SpreadLayoutTest, FiveWindowsFormTwoCenteredRows)
{
    std::vector<geom::Rectangle> const windows(5, geom::Rectangle {
                                                      { 0,   0   },
                                                      { 960, 540 }
    });
    auto const result = spread_layout::compute(BOUNDS, windows);
    ASSERT_EQ(result.size(), 5u);

    // Three on top, two beneath them.
    EXPECT_EQ(result[0].top_left.y, result[1].top_left.y);
    EXPECT_EQ(result[1].top_left.y, result[2].top_left.y);
    EXPECT_EQ(result[3].top_left.y, result[4].top_left.y);
    EXPECT_GT(result[3].top_left.y, result[0].top_left.y);

    // Both rows are centered on the bounds.
    int const center = (BOUNDS.top_left.x.as_value() + BOUNDS.right().as_value()) / 2;
    int const top_center = (result[0].top_left.x.as_value() + result[2].right().as_value()) / 2;
    int const bottom_center = (result[3].top_left.x.as_value() + result[4].right().as_value()) / 2;
    EXPECT_NEAR(top_center, center, 2);
    EXPECT_NEAR(bottom_center, center, 2);
}

TEST(SpreadLayoutTest, ThreeWindowsFormTwoThenOne)
{
    // Three windows tiled side by side across the output.
    std::vector<geom::Rectangle> const windows {
        geom::Rectangle { { 0, 0 },    { 640, 1080 } },
        geom::Rectangle { { 640, 0 },  { 640, 1080 } },
        geom::Rectangle { { 1280, 0 }, { 640, 1080 } },
    };
    auto const result = spread_layout::compute(BOUNDS, windows);
    ASSERT_EQ(result.size(), 3u);

    // Two on top, one beneath them.
    EXPECT_EQ(result[0].top_left.y, result[1].top_left.y);
    EXPECT_GT(result[2].top_left.y.as_value(), result[0].top_left.y.as_value());
    EXPECT_LT(result[0].top_left.x.as_value(), result[1].top_left.x.as_value());

    // The lone window in the last row is centered on the bounds.
    int const center = (BOUNDS.top_left.x.as_value() + BOUNDS.right().as_value()) / 2;
    int const last_center = (result[2].top_left.x.as_value() + result[2].right().as_value()) / 2;
    EXPECT_NEAR(last_center, center, 2);
}

TEST(SpreadLayoutTest, SingleWindowIsShrunkAndCentered)
{
    // A window that already fits the bounds is still scaled down: the spread is
    // an overview of the desktop, not a rearrangement of it.
    auto const result = spread_layout::compute(BOUNDS, {
                                                           geom::Rectangle { { 0, 0 }, { 1920, 1080 } }
    });
    ASSERT_EQ(result.size(), 1u);
    EXPECT_LT(result[0].size.width.as_value(), BOUNDS.size.width.as_value());
    EXPECT_LT(result[0].size.height.as_value(), BOUNDS.size.height.as_value());

    int const center_x = (BOUNDS.top_left.x.as_value() + BOUNDS.right().as_value()) / 2;
    int const center_y = (BOUNDS.top_left.y.as_value() + BOUNDS.bottom().as_value()) / 2;
    EXPECT_NEAR((result[0].top_left.x.as_value() + result[0].right().as_value()) / 2, center_x, 2);
    EXPECT_NEAR((result[0].top_left.y.as_value() + result[0].bottom().as_value()) / 2, center_y, 2);
}

TEST(SpreadLayoutTest, AspectRatioIsPreserved)
{
    std::vector<geom::Rectangle> const windows {
        geom::Rectangle { { 0, 0 }, { 1600, 900 } },
        geom::Rectangle { { 0, 0 }, { 400, 1000 } },
        geom::Rectangle { { 0, 0 }, { 700, 700 }  },
        geom::Rectangle { { 0, 0 }, { 1200, 300 } },
    };
    auto const result = spread_layout::compute(BOUNDS, windows);
    ASSERT_EQ(result.size(), windows.size());

    for (size_t i = 0; i < result.size(); ++i)
    {
        double const expected = static_cast<double>(windows[i].size.width.as_value()) / windows[i].size.height.as_value();
        double const actual = static_cast<double>(result[i].size.width.as_value()) / result[i].size.height.as_value();
        EXPECT_NEAR(actual, expected, 0.02) << "window " << i << " was distorted";
    }
}

TEST(SpreadLayoutTest, AlwaysScalesWindowsDown)
{
    // None of these come close to filling their cell, so nothing forces them to
    // shrink except the overview's own scale cap.
    std::vector<geom::Rectangle> const windows {
        geom::Rectangle { { 0, 0 }, { 320, 200 } },
        geom::Rectangle { { 0, 0 }, { 100, 400 } },
        geom::Rectangle { { 0, 0 }, { 640, 480 } },
    };
    auto const result = spread_layout::compute(BOUNDS, windows);
    ASSERT_EQ(result.size(), windows.size());

    // Matches MAX_SCALE in spread_layout.cpp, with a pixel of rounding slack.
    double const max_scale = 0.75;
    for (size_t i = 0; i < result.size(); ++i)
    {
        EXPECT_LE(result[i].size.width.as_value(), max_scale * windows[i].size.width.as_value() + 1)
            << "window " << i << " was not scaled down";
        EXPECT_LE(result[i].size.height.as_value(), max_scale * windows[i].size.height.as_value() + 1)
            << "window " << i << " was not scaled down";
    }
}

TEST(SpreadLayoutTest, TilesAreSeparatedByAtLeastTheGap)
{
    int const gap = 16;
    std::vector<geom::Rectangle> const windows(7, geom::Rectangle {
                                                      { 0,   0   },
                                                      { 960, 540 }
    });
    auto const result = spread_layout::compute(BOUNDS, windows, gap);
    ASSERT_EQ(result.size(), windows.size());

    // One pixel of slack for the rounding to integer coordinates.
    for (size_t i = 0; i < result.size(); ++i)
    {
        for (size_t j = i + 1; j < result.size(); ++j)
        {
            bool const clear_x = result[i].right().as_value() + gap - 1 <= result[j].top_left.x.as_value()
                || result[j].right().as_value() + gap - 1 <= result[i].top_left.x.as_value();
            bool const clear_y = result[i].bottom().as_value() + gap - 1 <= result[j].top_left.y.as_value()
                || result[j].bottom().as_value() + gap - 1 <= result[i].top_left.y.as_value();
            EXPECT_TRUE(clear_x || clear_y) << "windows " << i << " and " << j << " are closer than the gap";
        }
    }
}
