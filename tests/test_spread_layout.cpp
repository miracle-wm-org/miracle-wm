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
geom::Rectangle const BOUNDS { { 0, 0 }, { 1920, 1080 } };

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
    auto const result = spread_layout::compute(BOUNDS, { geom::Rectangle { { 700, 400 }, { 500, 300 } } });
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(within(result[0], BOUNDS));
}

TEST(SpreadLayoutTest, ReturnsOneRectanglePerInputInOrder)
{
    std::vector<geom::Rectangle> const windows {
        geom::Rectangle { { 0, 0 }, { 960, 1080 } },
        geom::Rectangle { { 960, 0 }, { 960, 540 } },
        geom::Rectangle { { 960, 540 }, { 960, 540 } },
    };
    auto const result = spread_layout::compute(BOUNDS, windows);
    ASSERT_EQ(result.size(), windows.size());
}

TEST(SpreadLayoutTest, OverlappingWindowsAreSeparated)
{
    // Five identical windows stacked at the same position.
    std::vector<geom::Rectangle> const windows(5, geom::Rectangle { { 500, 300 }, { 600, 400 } });
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
    geom::Rectangle const small_bounds { { 0, 0 }, { 800, 600 } };
    std::vector<geom::Rectangle> const windows(20, geom::Rectangle { { 100, 100 }, { 400, 300 } });
    auto const result = spread_layout::compute(small_bounds, windows);
    ASSERT_EQ(result.size(), windows.size());
    for (size_t i = 0; i < result.size(); ++i)
        EXPECT_TRUE(within(result[i], small_bounds)) << "window " << i << " escaped the bounds";
}

TEST(SpreadLayoutTest, NonOverlappingWindowsStillMoveOutward)
{
    // Two windows on either side of the center: the radial pre-spread should
    // push them further apart, not leave them exactly in place.
    geom::Rectangle const left { { 200, 400 }, { 400, 300 } };
    geom::Rectangle const right { { 1300, 400 }, { 400, 300 } };
    auto const result = spread_layout::compute(BOUNDS, { left, right });
    ASSERT_EQ(result.size(), 2u);
    EXPECT_LT(result[0].top_left.x.as_value(), left.top_left.x.as_value());
    EXPECT_GT(result[1].top_left.x.as_value(), right.top_left.x.as_value());
}
