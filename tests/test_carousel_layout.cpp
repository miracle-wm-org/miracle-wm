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
    auto const result = carousel_layout::compute(BOUNDS, input, 0.f);

    for (size_t i = 1; i < result.size(); i++)
        EXPECT_NEAR(options.dim, result[i].opacity, 0.001f) << "at index " << i;
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

TEST(CarouselLayout, WrapsAroundAfterAFullRevolution)
{
    auto const input = windows(5);
    auto const first = carousel_layout::compute(BOUNDS, input, 0.f);
    auto const wrapped = carousel_layout::compute(BOUNDS, input, static_cast<float>(input.size()));

    ASSERT_EQ(first.size(), wrapped.size());
    for (size_t i = 0; i < first.size(); i++)
    {
        EXPECT_NEAR(first[i].x, wrapped[i].x, 0.001f) << "at index " << i;
        EXPECT_NEAR(first[i].width, wrapped[i].width, 0.001f) << "at index " << i;
    }
}

TEST(CarouselLayout, TheLastWindowComesAroundToTheLeftOfTheFirst)
{
    auto const input = windows(5);
    auto const result = carousel_layout::compute(BOUNDS, input, 0.f);

    EXPECT_LT(center_x(result[4]), center_x(result[0]));
    EXPECT_GT(center_x(result[1]), center_x(result[0]));
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
    auto const result = carousel_layout::compute(BOUNDS, input, 0.f);

    // Three windows are wholly on screen...
    for (auto const index : { 0u, 1u, 4u })
    {
        EXPECT_GE(result[index].x, 0.f) << "at index " << index;
        EXPECT_LE(result[index].x + result[index].width, 1920.f) << "at index " << index;
    }

    // ...and the pair beyond them straddles an edge.
    EXPECT_LT(result[3].x, 0.f);
    EXPECT_GT(result[3].x + result[3].width, 0.f);
    EXPECT_LT(result[2].x, 1920.f);
    EXPECT_GT(result[2].x + result[2].width, 1920.f);
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
