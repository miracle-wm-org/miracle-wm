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

#include "mock_container.h"
#include "tiling_algorithms.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

namespace geom = mir::geometry;

using namespace miracle;
using namespace miracle::test;
using namespace testing;

class TestTilingAlgorithms : public testing::Test
{
};

TEST_F(TestTilingAlgorithms, TestAddingSingleVertical)
{
    geom::Rectangle container_area {
        { 0,   0   },
        { 100, 100 }
    };
    auto result = insert_node<true>({}, container_area, 0);

    EXPECT_THAT(result.top_left.x.as_int(), Eq(0));
    EXPECT_THAT(result.top_left.y.as_int(), Eq(0));
    EXPECT_THAT(result.size.width.as_int(), Eq(100));
    EXPECT_THAT(result.size.height.as_int(), Eq(100));
}

TEST_F(TestTilingAlgorithms, TestAddingSingleHorizontal)
{
    geom::Rectangle container_area {
        { 0,   0   },
        { 100, 100 }
    };
    auto result = insert_node<false>({}, container_area, 0);

    EXPECT_THAT(result.top_left.x.as_int(), Eq(0));
    EXPECT_THAT(result.top_left.y.as_int(), Eq(0));
    EXPECT_THAT(result.size.width.as_int(), Eq(100));
    EXPECT_THAT(result.size.height.as_int(), Eq(100));
}

TEST_F(TestTilingAlgorithms, TestAddingTwoVertical)
{
    geom::Rectangle container_area {
        { 0,   0   },
        { 100, 100 }
    };
    auto first = std::make_shared<MockContainer>();
    EXPECT_CALL(*first, get_logical_area())
        .WillRepeatedly(Return(container_area));
    EXPECT_CALL(*first, set_logical_area(_, true))
        .Times(2); // Called once to shrink, once to add remaining size

    std::vector<std::shared_ptr<Container>> containers = { first };
    auto result = insert_node<true>(containers, container_area, 0);

    EXPECT_THAT(result.top_left.y.as_int(), Eq(0));
    EXPECT_THAT(result.size.height.as_int(), Eq(50));
}

TEST_F(TestTilingAlgorithms, TestAddingTwoHorizontal)
{
    geom::Rectangle container_area {
        { 0,   0   },
        { 100, 100 }
    };
    auto first = std::make_shared<MockContainer>();
    EXPECT_CALL(*first, get_logical_area())
        .WillRepeatedly(Return(container_area));
    EXPECT_CALL(*first, set_logical_area(_, true))
        .Times(2); // Called once to shrink, once to add remaining size

    std::vector<std::shared_ptr<Container>> containers = { first };
    auto result = insert_node<false>(containers, container_area, 0);

    EXPECT_THAT(result.top_left.x.as_int(), Eq(0));
    EXPECT_THAT(result.size.width.as_int(), Eq(50));
}

TEST_F(TestTilingAlgorithms, TestAddingThreeVertical)
{
    geom::Rectangle container_area {
        { 0,   0   },
        { 100, 100 }
    };
    auto first = std::make_shared<MockContainer>();
    auto second = std::make_shared<MockContainer>();

    EXPECT_CALL(*first, get_logical_area())
        .WillRepeatedly(Return(geom::Rectangle {
            { 0,   0  },
            { 100, 50 }
    }));
    EXPECT_CALL(*first, set_logical_area(_, true))
        .Times(1);

    EXPECT_CALL(*second, get_logical_area())
        .WillRepeatedly(Return(geom::Rectangle {
            { 0,   50 },
            { 100, 50 }
    }));
    EXPECT_CALL(*second, set_logical_area(_, true))
        .Times(2); // Called once to shrink, once to add remaining size

    std::vector<std::shared_ptr<Container>> containers = { first, second };
    auto result = insert_node<true>(containers, container_area, 0);

    EXPECT_THAT(result.top_left.y.as_int(), Eq(0));
    EXPECT_THAT(result.size.height.as_int(), Eq(33));
}

TEST_F(TestTilingAlgorithms, TestAddingAtEnd)
{
    geom::Rectangle container_area {
        { 0,   0   },
        { 100, 100 }
    };
    auto first = std::make_shared<MockContainer>();
    auto second = std::make_shared<MockContainer>();
    auto third = std::make_shared<MockContainer>();

    EXPECT_CALL(*first, get_logical_area())
        .WillRepeatedly(Return(geom::Rectangle {
            { 0,   0  },
            { 100, 33 }
    }));
    EXPECT_CALL(*first, set_logical_area(_, true))
        .Times(1);

    EXPECT_CALL(*second, get_logical_area())
        .WillRepeatedly(Return(geom::Rectangle {
            { 0,   33 },
            { 100, 33 }
    }));
    EXPECT_CALL(*second, set_logical_area(_, true))
        .Times(1);

    EXPECT_CALL(*third, get_logical_area())
        .WillRepeatedly(Return(geom::Rectangle {
            { 0,   66 },
            { 100, 34 }
    }));
    EXPECT_CALL(*third, set_logical_area(_, true))
        .Times(1);

    std::vector<std::shared_ptr<Container>> containers = { first, second, third };
    auto result = insert_node<true>(containers, container_area, 4);

    EXPECT_THAT(result.top_left.y.as_int(), Eq(75));
    EXPECT_THAT(result.size.height.as_int(), Eq(25));
}
