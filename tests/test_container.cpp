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

#include "container.h"
#include "mock_container.h"
#include "mock_parent_container.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace miracle;
using namespace testing;

namespace
{
auto make_rectangle(int x, int y, int width, int height)
{
    return geom::Rectangle(geom::Point(x, y), geom::Size(width, height));
}
}

class ContainerExecuteResizeTest : public Test
{
public:
    ContainerExecuteResizeTest()
    {
        // Setup default behaviors for containers
        ON_CALL(*container, get_logical_area())
            .WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
        ON_CALL(*container, get_min_width())
            .WillByDefault(Return(50));
        ON_CALL(*container, get_min_height())
            .WillByDefault(Return(50));
        ON_CALL(*container, anchored())
            .WillByDefault(Return(true));

        // Allow mock leaks since these are member variables reused across tests
        Mock::AllowLeak(container.get());
        Mock::AllowLeak(north_neighbor.get());
        Mock::AllowLeak(south_neighbor.get());
        Mock::AllowLeak(east_neighbor.get());
        Mock::AllowLeak(west_neighbor.get());
    }

protected:
    std::shared_ptr<test::MockContainer> container = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> north_neighbor = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> south_neighbor = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> east_neighbor = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> west_neighbor = std::make_shared<NiceMock<test::MockContainer>>();

    void SetupNeighbor(
        std::shared_ptr<test::MockContainer> const& neighbor,
        geom::Rectangle const& area,
        size_t min_width = 50,
        size_t min_height = 50)
    {
        ON_CALL(*neighbor, get_logical_area())
            .WillByDefault(Return(area));
        ON_CALL(*neighbor, get_min_width())
            .WillByDefault(Return(min_width));
        ON_CALL(*neighbor, get_min_height())
            .WillByDefault(Return(min_height));
        ON_CALL(*neighbor, anchored())
            .WillByDefault(Return(true));
    }
};

// === North Edge Tests ===
TEST_F(ContainerExecuteResizeTest, ResizeNorthWithoutNeighborAndAnchored)
{
    // When resizing north without a north neighbor and the container is anchored,
    // nothing should happen
    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*container, commit_changes())
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthWithoutNeighborAndUnanchored)
{
    // When resizing north without a north neighbor and the container is unanchored,
    // the root should be resized
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored())
        .WillByDefault(Return(false));
    ON_CALL(*container, root())
        .WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area())
        .WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width())
        .WillByDefault(Return(50));
    ON_CALL(*root, get_min_height())
        .WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(100, 150, 400, 250), false))
        .Times(1);
    EXPECT_CALL(*root, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthExpandsCurrentAndShrinksNeighbor)
{
    // Setup: container at (100, 100, 400x300), north neighbor at (100, 0, 400x100)
    // The algorithm finds north, then gets south of north (which should return container)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));

    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south())
        .WillByDefault(Return(container));

    // Resize north by -50 (move top edge up by 50)
    // Container's current height is 300, resize_internal with mir_resize_edge_north and y_diff=-50
    // will set new_height = 300 + (-50) = 250, new_y = 100 + (300 - 250) = 150
    // So container becomes (100, 150, 400x250)
    // height_diff = 300 - 250 = 50
    // North neighbor resizes from south by height_diff=50, so new height = 100 + 50 = 150
    // North neighbor becomes (100, 0, 400x150)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 150, 400, 250), false))
        .Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 150), false))
        .Times(1);
    EXPECT_CALL(*container, commit_changes())
        .Times(1);
    EXPECT_CALL(*north_neighbor, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthStoppedByMinimumSize)
{
    // Setup: north neighbor is already at minimum size (50x50)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 50), 50, 50);

    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south())
        .WillByDefault(Return(container));

    // Try to resize north by -60, which would require shrinking north neighbor below minimum
    // Container resizes: new_height = 300 + (-60) = 240, new_y = 100 + (300 - 240) = 160
    // Container becomes (100, 160, 400x240)
    // height_diff = 300 - 240 = 60
    // North neighbor tries to resize from south by 60: new_height = 50 + 60 = 110
    // Since this doesn't violate minimum (110 >= 50), north neighbor becomes (100, 0, 400x110)
    // This test should actually succeed. Let's test the opposite: north neighbor expanding beyond what we can shrink
    // Actually, to stop resize, we need the neighbor to be clamped when it tries to shrink
    // Let's try: resize by +260, container tries to shrink to 40 (below min 50), gets clamped at 50
    // height_diff = 300 - 50 = 250
    // North neighbor tries to resize by -250: new_height = 50 + (-250) = -200, clamped to 50
    // This should trigger the clamped flag
    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*north_neighbor, set_logical_area(_, _))
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, 260, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthShrinkCurrentAndExpandNeighbor)
{
    // Setup: container at (100, 100, 400x300), north neighbor at (100, 0, 400x100)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));

    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south())
        .WillByDefault(Return(container));

    // Resize north by 50 (move top edge down by 50)
    // Container's new height = 300 + 50 = 350, new_y = 100 + (300 - 350) = 50
    // Container becomes (100, 50, 400x350)
    // height_diff = 300 - 350 = -50
    // North neighbor resizes from south by -50, new height = 100 + (-50) = 50
    // North neighbor becomes (100, 0, 400x50)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 50, 400, 350), false))
        .Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 50), false))
        .Times(1);
    EXPECT_CALL(*container, commit_changes())
        .Times(1);
    EXPECT_CALL(*north_neighbor, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, 50, false);
}

// === South Edge Tests ===
TEST_F(ContainerExecuteResizeTest, ResizeSouthWithoutNeighborAndAnchored)
{
    ON_CALL(*container, neighbor_south())
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*container, commit_changes())
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_south, 0, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthWithoutNeighborAndUnanchored)
{
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_south())
        .WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored())
        .WillByDefault(Return(false));
    ON_CALL(*container, root())
        .WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area())
        .WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width())
        .WillByDefault(Return(50));
    ON_CALL(*root, get_min_height())
        .WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(100, 100, 400, 350), false))
        .Times(1);
    EXPECT_CALL(*root, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_south, 0, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthExpandsCurrentAndShrinksNeighbor)
{
    // Setup: container at (100, 100, 400x300), south neighbor at (100, 400, 400x100)
    // Algorithm: find south, then get north of south (returns container)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 100));

    ON_CALL(*container, neighbor_south())
        .WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north())
        .WillByDefault(Return(container));

    // Resize south by 50 (move bottom edge down)
    // Container's new height = 300 + 50 = 350
    // Container becomes (100, 100, 400x350)
    // height_diff = 300 - 350 = -50
    // South neighbor resizes from north by -50, new height = 100 + (-50) = 50, new_y = 400 + (100 - 50) = 450
    // South neighbor becomes (100, 450, 400x50)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 400, 350), false))
        .Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(make_rectangle(100, 450, 400, 50), false))
        .Times(1);
    EXPECT_CALL(*container, commit_changes())
        .Times(1);
    EXPECT_CALL(*south_neighbor, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_south, 0, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthStoppedByMinimumSize)
{
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 50), 50, 50);

    ON_CALL(*container, neighbor_south())
        .WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north())
        .WillByDefault(Return(container));

    // Try to expand beyond what south neighbor can shrink
    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*south_neighbor, set_logical_area(_, _))
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_south, 0, 100, false);
}

// === East Edge Tests ===
TEST_F(ContainerExecuteResizeTest, ResizeEastWithoutNeighborAndAnchored)
{
    ON_CALL(*container, neighbor_east())
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*container, commit_changes())
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_east, 50, 0, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeEastWithoutNeighborAndUnanchored)
{
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_east())
        .WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored())
        .WillByDefault(Return(false));
    ON_CALL(*container, root())
        .WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area())
        .WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width())
        .WillByDefault(Return(50));
    ON_CALL(*root, get_min_height())
        .WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(100, 100, 450, 300), false))
        .Times(1);
    EXPECT_CALL(*root, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_east, 50, 0, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeEastExpandsCurrentAndShrinksNeighbor)
{
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 100, 300));

    ON_CALL(*container, neighbor_east())
        .WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west())
        .WillByDefault(Return(container));

    // Resize east by 50 (move right edge right)
    // Container's new width = 400 + 50 = 450
    // Container becomes (100, 100, 450x300)
    // width_diff = 400 - 450 = -50
    // East neighbor resizes from west by -50, new width = 100 + (-50) = 50, new_x = 500 + (100 - 50) = 550
    // East neighbor becomes (550, 100, 50x300)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 450, 300), false))
        .Times(1);
    EXPECT_CALL(*east_neighbor, set_logical_area(make_rectangle(550, 100, 50, 300), false))
        .Times(1);
    EXPECT_CALL(*container, commit_changes())
        .Times(1);
    EXPECT_CALL(*east_neighbor, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_east, 50, 0, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeEastStoppedByMinimumSize)
{
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 50, 300), 50, 50);

    ON_CALL(*container, neighbor_east())
        .WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west())
        .WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*east_neighbor, set_logical_area(_, _))
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_east, 100, 0, false);
}

// === West Edge Tests ===
TEST_F(ContainerExecuteResizeTest, ResizeWestWithoutNeighborAndAnchored)
{
    ON_CALL(*container, neighbor_west())
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*container, commit_changes())
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_west, -50, 0, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeWestWithoutNeighborAndUnanchored)
{
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_west())
        .WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored())
        .WillByDefault(Return(false));
    ON_CALL(*container, root())
        .WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area())
        .WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width())
        .WillByDefault(Return(50));
    ON_CALL(*root, get_min_height())
        .WillByDefault(Return(50));

    // Resize west with x=-50 means move left edge left
    // new_width = 400 + (-50) = 350, new_x = 100 + (400 - 350) = 150
    EXPECT_CALL(*root, set_logical_area(make_rectangle(150, 100, 350, 300), false))
        .Times(1);
    EXPECT_CALL(*root, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_west, -50, 0, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeWestExpandsCurrentAndShrinksNeighbor)
{
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300));

    ON_CALL(*container, neighbor_west())
        .WillByDefault(Return(west_neighbor));
    // Note: there's likely a bug in container.cpp:317 - it calls neighbor_west() twice
    // For this test to pass with current code, we need to return container for neighbor_west of west_neighbor
    ON_CALL(*west_neighbor, neighbor_east())
        .WillByDefault(Return(container));

    // Resize west by -50 (move left edge left)
    // Container's new width = 400 + (-50) = 350, new_x = 100 + (400 - 350) = 150
    // Container becomes (150, 100, 350x300)
    // width_diff = 400 - 350 = 50
    // West neighbor resizes from east by 50, new width = 100 + 50 = 150
    // West neighbor becomes (0, 100, 150x300)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(150, 100, 350, 300), false))
        .Times(1);
    EXPECT_CALL(*west_neighbor, set_logical_area(make_rectangle(0, 100, 150, 300), false))
        .Times(1);
    EXPECT_CALL(*container, commit_changes())
        .Times(1);
    EXPECT_CALL(*west_neighbor, commit_changes())
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_west, -50, 0, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeWestStoppedByMinimumSize)
{
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 50, 300), 50, 50);

    ON_CALL(*container, neighbor_west())
        .WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east())
        .WillByDefault(Return(container));

    // Try to resize west by +360, container tries to shrink to 40 (below min 50), gets clamped at 50
    // width_diff = 400 - 50 = 350
    // West neighbor tries to resize from east by 350: new_width = 50 + 350 = 400
    // This doesn't violate minimum. We need to test the opposite direction.
    // Actually we need west neighbor to be clamped when it tries to shrink
    // Try resizing by -360: container expands to new_width = 400 + (-360) = 40, clamped to 50
    // wait, that makes container smaller...
    // Let's think: container new_width = 400 + (-360) = 40, clamped to 50, new_x = 100 + (400-50) = 450
    // Container becomes (450, 100, 50x300)
    // width_diff = 400 - 50 = 350
    // West neighbor tries east resize by 350: new_width = 50 + 350 = 400
    // Still no clamping. The issue is that for west edge the logic seems wrong.
    // Let me test when we push container RIGHT (positive x) to try to take space from west
    // container new_width = 400 + 360 = 760, new_x = 100 + (400 - 760) = -260
    // Container becomes (-260, 100, 760x300)
    // width_diff = 400 - 760 = -360
    // West neighbor tries east resize by -360: new_width = 50 + (-360) = -310, clamped to 50
    // This should trigger the clamped flag!
    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*west_neighbor, set_logical_area(_, _))
        .Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_west, 360, 0, false);
}

// === Animation Tests ===
TEST_F(ContainerExecuteResizeTest, ResizeNorthWithAnimations)
{
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));

    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south())
        .WillByDefault(Return(container));

    // Verify that with_animations parameter is passed through
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 150, 400, 250), true))
        .Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 150), true))
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, -50, true);
}

// === Minimum Size Clamping Tests ===
TEST_F(ContainerExecuteResizeTest, ResizeNorthClampedToMinimumHeight)
{
    // Container at minimum height tries to shrink further
    ON_CALL(*container, get_logical_area())
        .WillByDefault(Return(make_rectangle(100, 100, 400, 50)));

    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));

    ON_CALL(*container, neighbor_north())
        .WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south())
        .WillByDefault(Return(container));

    // Try to shrink by 30 pixels - container is at min height (50)
    // new_height would be 50 + (-30) = 20, but gets clamped to 50
    // So container stays at (100, 100, 400x50)
    // height_diff = 50 - 50 = 0
    // North neighbor resizes by 0, stays at (100, 0, 400x100)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 400, 50), false))
        .Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 100), false))
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_north, 0, -30, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeEastClampedToMinimumWidth)
{
    // Container at minimum width tries to shrink further
    ON_CALL(*container, get_logical_area())
        .WillByDefault(Return(make_rectangle(100, 100, 50, 300)));

    SetupNeighbor(east_neighbor, make_rectangle(150, 100, 100, 300));

    ON_CALL(*container, neighbor_east())
        .WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west())
        .WillByDefault(Return(container));

    // Try to shrink by 30 pixels - container is at min width (50)
    // new_width would be 50 + (-30) = 20, but gets clamped to 50
    // Container stays at (100, 100, 50x300)
    // width_diff = 50 - 50 = 0
    // East neighbor resizes by 0, stays at (150, 100, 100x300)
    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 50, 300), false))
        .Times(1);
    EXPECT_CALL(*east_neighbor, set_logical_area(make_rectangle(150, 100, 100, 300), false))
        .Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_east, -30, 0, false);
}

// === Diagonal (Corner) Edge Tests ===
//
// For all diagonal tests the default container area is (100, 100, 400x300), min 50x50.
//
// Neighbor areas:
//   north: (100, 0, 400x100)    south: (100, 400, 400x100)
//   east:  (500, 100, 100x300)  west:  (0, 100, 100x300)

// --- Northeast ---

TEST_F(ContainerExecuteResizeTest, ResizeNortheastNoNeighborsAnchored)
{
    ON_CALL(*container, neighbor_north()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(true));

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*container, commit_changes()).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_northeast, 50, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNortheastNoNeighborsUnanchored)
{
    // No tiling neighbors → resize the floating root using the combined diagonal edge.
    // resize_internal(root, northeast, 50, -50):
    //   set_north: height=300+(-50)=250, y=100+(300-250)=150
    //   set_east:  width=400+50=450
    //   → (100, 150, 450x250)
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(false));
    ON_CALL(*container, root()).WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area()).WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width()).WillByDefault(Return(50));
    ON_CALL(*root, get_min_height()).WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(100, 150, 450, 250), false)).Times(1);
    EXPECT_CALL(*root, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_northeast, 50, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNortheastBothNeighbors)
{
    // north at (100, 0, 400x100), east at (500, 100, 150x300).
    // east uses width=150 so it shrinks to 100 (above minimum 50) and is not clamped.
    //
    // North axis (y=-50):
    //   ry = resize_internal(container, north, 0, -50): height=250, y=150 → (100,150,400x250)
    //   height_diff = 300-250 = 50
    //   ny_result = resize_internal(north, south, 0, 50): height=150 → (100,0,400x150). Not clamped.
    //
    // East axis (x=50):
    //   rx = resize_internal(container, east, 50, 0): width=450 → (100,100,450x300)
    //   width_diff = 400-450 = -50
    //   nx_result = resize_internal(east, west, -50, 0):
    //     set_west: new_width=150-50=100, new_x=500+(150-100)=550 → (550,100,100x300). Not clamped.
    //
    // Combined container rect: x=100 (rx.x, east unchanged), y=150 (ry.y), w=450, h=250
    //   → (100, 150, 450x250)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 150, 300));

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 150, 450, 250), false)).Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 150), false)).Times(1);
    EXPECT_CALL(*east_neighbor, set_logical_area(make_rectangle(550, 100, 100, 300), false)).Times(1);
    EXPECT_CALL(*container, commit_changes()).Times(1);
    EXPECT_CALL(*north_neighbor, commit_changes()).Times(1);
    EXPECT_CALL(*east_neighbor, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_northeast, 50, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNortheastNorthNeighborClamped)
{
    // North neighbor at minimum height (50). y=260 causes north to expand (container grows north),
    // which tries to shrink north neighbor below minimum → north axis aborts, east still proceeds.
    // east uses width=150 so it shrinks to 100 (above minimum 50) and is not clamped.
    //
    // North axis (y=260):
    //   ry: height=300+260=560, y=100+(300-560)=-160
    //   height_diff = 300-560 = -260
    //   ny_result: north height=100+(-260)=-160, clamped to 50. y_ok=false.
    //
    // East axis (x=50):
    //   rx: width=450 → not clamped. x_ok=true.
    //   nx_result: set_west: new_width=150-50=100, new_x=550 → not clamped.
    //
    // Combined: north axis skipped → y=100 (original), height=300 (original), width=450
    //   → (100, 100, 450x300)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100), 50, 50);
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 150, 300));

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 450, 300), false)).Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*east_neighbor, set_logical_area(make_rectangle(550, 100, 100, 300), false)).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_northeast, 50, 260, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNortheastEastNeighborClamped)
{
    // East neighbor at minimum width (50). x=260 tries to expand container east,
    // which would shrink east neighbor below minimum → east axis aborts, north still proceeds.
    //
    // North axis (y=-50): not clamped. y_ok=true.
    // East axis (x=260):
    //   rx: width=400+260=660 → width_diff=400-660=-260
    //   nx_result: east width=100+(-260)=-160, clamped. x_ok=false.
    //
    // Combined: east axis skipped → x=100, width=400 (original); north axis applied
    //   → (100, 150, 400x250)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 100, 300), 50, 50);

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 150, 400, 250), false)).Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 150), false)).Times(1);
    EXPECT_CALL(*east_neighbor, set_logical_area(_, _)).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_northeast, 260, -50, false);
}

// --- Northwest ---

TEST_F(ContainerExecuteResizeTest, ResizeNorthwestNoNeighborsAnchored)
{
    ON_CALL(*container, neighbor_north()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(true));

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*container, commit_changes()).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_northwest, -50, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthwestNoNeighborsUnanchored)
{
    // resize_internal(root, northwest, -50, -50):
    //   set_north: height=300+(-50)=250, y=150
    //   set_west:  width=400+(-50)=350, x=100+(400-350)=150
    //   → (150, 150, 350x250)
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(false));
    ON_CALL(*container, root()).WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area()).WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width()).WillByDefault(Return(50));
    ON_CALL(*root, get_min_height()).WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(150, 150, 350, 250), false)).Times(1);
    EXPECT_CALL(*root, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_northwest, -50, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthwestBothNeighbors)
{
    // north at (100, 0, 400x100), west at (0, 100, 100x300).
    //
    // North axis (y=-50): ry=(100,150,400x250), height_diff=50, north→(100,0,400x150). OK.
    // West axis (x=-50):
    //   rx = resize_internal(container, west, -50, 0): width=350, x=150 → (150,100,350x300)
    //   width_diff = 400-350 = 50
    //   nx_result = resize_internal(west, east, 50, 0): width=150 → (0,100,150x300). OK.
    //
    // Combined: x=150(rx), y=150(ry), w=350, h=250 → (150, 150, 350x250)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300));

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(150, 150, 350, 250), false)).Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 150), false)).Times(1);
    EXPECT_CALL(*west_neighbor, set_logical_area(make_rectangle(0, 100, 150, 300), false)).Times(1);
    EXPECT_CALL(*container, commit_changes()).Times(1);
    EXPECT_CALL(*north_neighbor, commit_changes()).Times(1);
    EXPECT_CALL(*west_neighbor, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_northwest, -50, -50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthwestNorthNeighborClamped)
{
    // y=260 causes north axis to clamp; west proceeds.
    //
    // North axis (y=260): height_diff=-260, north clamped. y_ok=false.
    // West axis (x=-50): rx=(150,100,350x300), width_diff=50, west→(0,100,150x300). OK.
    //
    // Combined: y=100 (original), h=300 (original), x=150, w=350 → (150, 100, 350x300)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100), 50, 50);
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300));

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(150, 100, 350, 300), false)).Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*west_neighbor, set_logical_area(make_rectangle(0, 100, 150, 300), false)).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_northwest, -50, 260, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeNorthwestWestNeighborClamped)
{
    // x=260 (move left edge left, expanding container west) causes west to shrink below min.
    // West axis (x=260): rx width=660, width_diff=-260, west clamped. x_ok=false.
    // North axis (y=-50): OK.
    //
    // Combined: x=100 (original), w=400 (original), y=150, h=250 → (100, 150, 400x250)
    SetupNeighbor(north_neighbor, make_rectangle(100, 0, 400, 100));
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300), 50, 50);

    ON_CALL(*container, neighbor_north()).WillByDefault(Return(north_neighbor));
    ON_CALL(*north_neighbor, neighbor_south()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 150, 400, 250), false)).Times(1);
    EXPECT_CALL(*north_neighbor, set_logical_area(make_rectangle(100, 0, 400, 150), false)).Times(1);
    EXPECT_CALL(*west_neighbor, set_logical_area(_, _)).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_northwest, 260, -50, false);
}

// --- Southeast ---

TEST_F(ContainerExecuteResizeTest, ResizeSoutheastNoNeighborsAnchored)
{
    ON_CALL(*container, neighbor_south()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(true));

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*container, commit_changes()).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_southeast, 50, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSoutheastNoNeighborsUnanchored)
{
    // resize_internal(root, southeast, 50, 50):
    //   set_south: height=300+50=350 → y unchanged
    //   set_east:  width=400+50=450
    //   → (100, 100, 450x350)
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(false));
    ON_CALL(*container, root()).WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area()).WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width()).WillByDefault(Return(50));
    ON_CALL(*root, get_min_height()).WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(100, 100, 450, 350), false)).Times(1);
    EXPECT_CALL(*root, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_southeast, 50, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSoutheastBothNeighbors)
{
    // south at (100, 400, 400x100), east at (500, 100, 100x300).
    //
    // South axis (y=50):
    //   ry = resize_internal(container, south, 0, 50): height=350, y=100 → (100,100,400x350)
    //   height_diff = 300-350 = -50
    //   ny_result = resize_internal(south, north, 0, -50): height=50, y=450 → (100,450,400x50). OK.
    //
    // East axis (x=50): rx=(100,100,450x300), width_diff=-50, east→(550,100,50x300). OK.
    //
    // Combined: x=100(rx), y=100(ry), w=450, h=350 → (100, 100, 450x350)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 100));
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 100, 300));

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 450, 350), false)).Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(make_rectangle(100, 450, 400, 50), false)).Times(1);
    EXPECT_CALL(*east_neighbor, set_logical_area(make_rectangle(550, 100, 50, 300), false)).Times(1);
    EXPECT_CALL(*container, commit_changes()).Times(1);
    EXPECT_CALL(*south_neighbor, commit_changes()).Times(1);
    EXPECT_CALL(*east_neighbor, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_southeast, 50, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSoutheastSouthNeighborClamped)
{
    // y=100 expands container south, shrinks south neighbor below minimum. y_ok=false.
    // East (x=50) still proceeds.
    //
    // Combined: y=100 (original), h=300 (original), x=100, w=450 → (100, 100, 450x300)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 50), 50, 50);
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 100, 300));

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 450, 300), false)).Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*east_neighbor, set_logical_area(make_rectangle(550, 100, 50, 300), false)).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_southeast, 50, 100, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSoutheastEastNeighborClamped)
{
    // x=260 shrinks east neighbor below minimum. x_ok=false. South (y=50) still proceeds.
    //
    // Combined: x=100 (original), w=400 (original), y=100, h=350 → (100, 100, 400x350)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 100));
    SetupNeighbor(east_neighbor, make_rectangle(500, 100, 100, 300), 50, 50);

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_east()).WillByDefault(Return(east_neighbor));
    ON_CALL(*east_neighbor, neighbor_west()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 400, 350), false)).Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(make_rectangle(100, 450, 400, 50), false)).Times(1);
    EXPECT_CALL(*east_neighbor, set_logical_area(_, _)).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_southeast, 260, 50, false);
}

// --- Southwest ---

TEST_F(ContainerExecuteResizeTest, ResizeSouthwestNoNeighborsAnchored)
{
    ON_CALL(*container, neighbor_south()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(true));

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*container, commit_changes()).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_southwest, -50, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthwestNoNeighborsUnanchored)
{
    // resize_internal(root, southwest, -50, 50):
    //   set_south: height=350
    //   set_west:  width=350, x=150
    //   → (150, 100, 350x350)
    auto root = std::make_shared<NiceMock<test::MockContainer>>();
    Mock::AllowLeak(root.get());

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(nullptr));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(nullptr));
    ON_CALL(*container, anchored()).WillByDefault(Return(false));
    ON_CALL(*container, root()).WillByDefault(Return(root));
    ON_CALL(*root, get_logical_area()).WillByDefault(Return(make_rectangle(100, 100, 400, 300)));
    ON_CALL(*root, get_min_width()).WillByDefault(Return(50));
    ON_CALL(*root, get_min_height()).WillByDefault(Return(50));

    EXPECT_CALL(*root, set_logical_area(make_rectangle(150, 100, 350, 350), false)).Times(1);
    EXPECT_CALL(*root, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_southwest, -50, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthwestBothNeighbors)
{
    // south at (100, 400, 400x150), west at (0, 100, 100x300).
    // south uses height=150 so it shrinks to 100 (above minimum 50) and is not clamped.
    //
    // South axis (y=50): ry=(100,100,400x350), height_diff=-50
    //   ny_result: set_north: new_height=150-50=100, new_y=400+(150-100)=450 → (100,450,400x100). OK.
    // West axis (x=-50): rx=(150,100,350x300), width_diff=50, west→(0,100,150x300). OK.
    //
    // Combined: x=150(rx), y=100(ry), w=350, h=350 → (150, 100, 350x350)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 150));
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300));

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(150, 100, 350, 350), false)).Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(make_rectangle(100, 450, 400, 100), false)).Times(1);
    EXPECT_CALL(*west_neighbor, set_logical_area(make_rectangle(0, 100, 150, 300), false)).Times(1);
    EXPECT_CALL(*container, commit_changes()).Times(1);
    EXPECT_CALL(*south_neighbor, commit_changes()).Times(1);
    EXPECT_CALL(*west_neighbor, commit_changes()).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_southwest, -50, 50, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthwestSouthNeighborClamped)
{
    // y=100 tries to shrink south neighbor below min. y_ok=false. West proceeds.
    //
    // Combined: y=100 (original), h=300 (original), x=150, w=350 → (150, 100, 350x300)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 50), 50, 50);
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300));

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(150, 100, 350, 300), false)).Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(_, _)).Times(0);
    EXPECT_CALL(*west_neighbor, set_logical_area(make_rectangle(0, 100, 150, 300), false)).Times(1);

    Container::execute_resize(container.get(), mir_resize_edge_southwest, -50, 100, false);
}

TEST_F(ContainerExecuteResizeTest, ResizeSouthwestWestNeighborClamped)
{
    // x=260 tries to shrink west neighbor below min. x_ok=false. South proceeds.
    //
    // Combined: x=100 (original), w=400 (original), y=100, h=350 → (100, 100, 400x350)
    SetupNeighbor(south_neighbor, make_rectangle(100, 400, 400, 100));
    SetupNeighbor(west_neighbor, make_rectangle(0, 100, 100, 300), 50, 50);

    ON_CALL(*container, neighbor_south()).WillByDefault(Return(south_neighbor));
    ON_CALL(*south_neighbor, neighbor_north()).WillByDefault(Return(container));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(west_neighbor));
    ON_CALL(*west_neighbor, neighbor_east()).WillByDefault(Return(container));

    EXPECT_CALL(*container, set_logical_area(make_rectangle(100, 100, 400, 350), false)).Times(1);
    EXPECT_CALL(*south_neighbor, set_logical_area(make_rectangle(100, 450, 400, 50), false)).Times(1);
    EXPECT_CALL(*west_neighbor, set_logical_area(_, _)).Times(0);

    Container::execute_resize(container.get(), mir_resize_edge_southwest, 260, 50, false);
}

// === None Edge Test ===
TEST_F(ContainerExecuteResizeTest, ResizeNoneEdgeDoesNothing)
{
    EXPECT_CALL(*container, set_logical_area(_, _))
        .Times(0);
    EXPECT_CALL(*container, commit_changes())
        .Times(0);

    // Using mir_resize_edge_none (the default/no resize edge)
    Container::execute_resize(container.get(), mir_resize_edge_none, 50, 50, false);
}
