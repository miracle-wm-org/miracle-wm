/**
Copyright (C) 2024  Matthew Kosarek

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

#include "tiling_algorithms.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

class TestTilingAlgorithms : public testing::Test
{
};

TEST_F(TestTilingAlgorithms, TestAddingSingle)
{
    auto result = insert_node({}, 100, 0, 0);
    EXPECT_THAT(result.positions.size(), Eq(1));
    EXPECT_THAT(result.positions[0].size, Eq(100));
    EXPECT_THAT(result.positions[0].position, Eq(0));
}

TEST_F(TestTilingAlgorithms, TestAddingTwo)
{
    auto first = insert_node({}, 100, 0, 0);
    auto result = insert_node(first.positions, 100, 0, 0);
    EXPECT_THAT(result.positions.size(), Eq(2));
    EXPECT_THAT(result.positions[0].size, Eq(50));
    EXPECT_THAT(result.positions[0].position, Eq(0));
    EXPECT_THAT(result.positions[1].size, Eq(50));
    EXPECT_THAT(result.positions[1].position, Eq(50));
}

TEST_F(TestTilingAlgorithms, TestAddingThree)
{
    auto first = insert_node({}, 100, 0, 0);
    auto second = insert_node(first.positions, 100, 0, 0);
    auto result = insert_node(second.positions, 100, 0, 0);

    EXPECT_THAT(result.positions.size(), Eq(3));
    EXPECT_THAT(result.positions[0].size, Eq(33));
    EXPECT_THAT(result.positions[0].position, Eq(0));
    EXPECT_THAT(result.positions[1].size, Eq(33.5));
    EXPECT_THAT(result.positions[1].position, Eq(33));
    EXPECT_THAT(result.positions[2].size, Eq(33.5));
    EXPECT_THAT(result.positions[2].position, Eq(66.5));
}

TEST_F(TestTilingAlgorithms, TestAddingFour)
{
    auto first = insert_node({}, 100, 0, 0);
    auto second = insert_node(first.positions, 100, 0, 0);
    auto third = insert_node(second.positions, 100, 0, 0);
    auto result = insert_node(third.positions, 100, 0, 4);

    EXPECT_THAT(result.positions.size(), Eq(4));
    EXPECT_THAT(result.positions[0].size, Eq(24.75));
    EXPECT_THAT(result.positions[0].position, Eq(0));
    EXPECT_THAT(result.positions[1].size, Eq(25.125));
    EXPECT_THAT(result.positions[1].position, Eq(24.75));
    EXPECT_THAT(result.positions[2].size, Eq(25.125));
    EXPECT_THAT(result.positions[2].position, Eq(49.875));
    EXPECT_THAT(result.positions[3].size, Eq(25));
    EXPECT_THAT(result.positions[3].position, Eq(75));
}
