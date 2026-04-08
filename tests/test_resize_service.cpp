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

#include "compositor_state.h"
#include "mock_command_controller.h"
#include "mock_container.h"
#include "resize_service.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
geom::Rectangle const default_area {
    { 100, 200 },
    { 400, 300 }
};
}

class ResizeServiceTest : public ::testing::Test
{
public:
    ResizeServiceTest() :
        command_controller(std::make_shared<NiceMock<test::MockCommandController>>()),
        service(command_controller, nullptr, nullptr, nullptr)
    {
    }

    std::shared_ptr<NiceMock<test::MockContainer>> make_floating(
        geom::Rectangle area = default_area)
    {
        auto container = std::make_shared<NiceMock<test::MockContainer>>();
        ON_CALL(*container, anchored()).WillByDefault(Return(false));
        ON_CALL(*container, get_logical_area()).WillByDefault(Return(area));
        ON_CALL(*container, get_min_width()).WillByDefault(Return(50));
        ON_CALL(*container, get_min_height()).WillByDefault(Return(50));
        return container;
    }

    std::shared_ptr<NiceMock<test::MockContainer>> make_tiled(
        geom::Rectangle area = default_area)
    {
        auto container = std::make_shared<NiceMock<test::MockContainer>>();
        ON_CALL(*container, anchored()).WillByDefault(Return(true));
        ON_CALL(*container, get_logical_area()).WillByDefault(Return(area));
        ON_CALL(*container, get_min_width()).WillByDefault(Return(50));
        ON_CALL(*container, get_min_height()).WillByDefault(Return(50));
        ON_CALL(*container, neighbor_north()).WillByDefault(Return(nullptr));
        ON_CALL(*container, neighbor_south()).WillByDefault(Return(nullptr));
        ON_CALL(*container, neighbor_east()).WillByDefault(Return(nullptr));
        ON_CALL(*container, neighbor_west()).WillByDefault(Return(nullptr));
        return container;
    }

    void start_resize(std::shared_ptr<WindowContainer> const& container,
        MirResizeEdge edge,
        float x = 200.f,
        float y = 300.f)
    {
        service.handle_request_resize(container, mir_pointer_action_button_down, edge, x, y);
    }

protected:
    std::shared_ptr<NiceMock<test::MockCommandController>> command_controller;
    ResizeService service;
};

// ---- Guard: not resizing ----

TEST_F(ResizeServiceTest, NotResizing_HandlePointerEvent_ReturnsFalse)
{
    EXPECT_FALSE(service.handle_pointer_event(100.f, 100.f, mir_pointer_action_motion));
}

// ---- handle_request_resize activation ----

TEST_F(ResizeServiceTest, HandleRequestResize_ButtonDown_StartsResizing)
{
    auto container = make_floating();
    EXPECT_CALL(*command_controller, set_mode(WindowManagerMode::resizing)).Times(1);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east, 200.f, 300.f);

    EXPECT_TRUE(service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, HandleRequestResize_ButtonUp_DoesNotStartResizing)
{
    auto container = make_floating();
    EXPECT_CALL(*command_controller, set_mode(_)).Times(0);

    service.handle_request_resize(container, mir_pointer_action_button_up, mir_resize_edge_east, 200.f, 300.f);

    EXPECT_FALSE(service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, HandleRequestResize_SecondButtonDown_WhenAlreadyResizing_Ignored)
{
    auto container1 = make_floating();
    auto container2 = make_floating();

    EXPECT_CALL(*command_controller, set_mode(WindowManagerMode::resizing)).Times(1);

    service.handle_request_resize(container1, mir_pointer_action_button_down, mir_resize_edge_east, 200.f, 300.f);
    service.handle_request_resize(container2, mir_pointer_action_button_down, mir_resize_edge_west, 200.f, 300.f);
}

// ---- Stop behavior ----

TEST_F(ResizeServiceTest, HandlePointerEvent_ButtonUp_StopsResizingAndResetsMode)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_east);

    EXPECT_CALL(*command_controller, set_mode(WindowManagerMode::normal)).Times(1);
    EXPECT_FALSE(service.handle_pointer_event(200.f, 300.f, mir_pointer_action_button_up));

    EXPECT_FALSE(service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, HandlePointerEvent_NonMotion_ReturnsFalse)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_east);

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);
    EXPECT_FALSE(service.handle_pointer_event(200.f, 300.f, mir_pointer_action_button_down));
}

TEST_F(ResizeServiceTest, HandlePointerEvent_MotionZeroDelta_ReturnsTrueNoResize)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_east, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);
    EXPECT_TRUE(service.handle_pointer_event(200.f, 300.f, mir_pointer_action_motion));
}

// ---- Floating: cardinal edge motion ----

TEST_F(ResizeServiceTest, FloatingWindow_EastEdge_MotionRight_IncreasesWidth)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_east, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 100
            && r.top_left.y.as_int() == 200
            && r.size.width.as_int() == 410
            && r.size.height.as_int() == 300;
    }),
                                false))
        .Times(1);
    EXPECT_CALL(*container, commit_changes()).Times(1);

    EXPECT_TRUE(service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, FloatingWindow_WestEdge_MotionRight_DecreasesWidthMovesX)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_west, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 110
            && r.top_left.y.as_int() == 200
            && r.size.width.as_int() == 390
            && r.size.height.as_int() == 300;
    }),
                                false))
        .Times(1);

    EXPECT_TRUE(service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, FloatingWindow_SouthEdge_MotionDown_IncreasesHeight)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_south, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 100
            && r.top_left.y.as_int() == 200
            && r.size.width.as_int() == 400
            && r.size.height.as_int() == 315;
    }),
                                false))
        .Times(1);

    EXPECT_TRUE(service.handle_pointer_event(200.f, 315.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, FloatingWindow_NorthEdge_MotionDown_DecreasesHeightMovesY)
{
    auto container = make_floating();
    start_resize(container, mir_resize_edge_north, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 100
            && r.top_left.y.as_int() == 215
            && r.size.width.as_int() == 400
            && r.size.height.as_int() == 285;
    }),
                                false))
        .Times(1);

    EXPECT_TRUE(service.handle_pointer_event(200.f, 315.f, mir_pointer_action_motion));
}

// ---- Floating: minimum size clamping ----

TEST_F(ResizeServiceTest, FloatingWindow_EastEdge_MotionLeftBeyondMin_ClampsWidth)
{
    // width=60, min=50; dx=-15 would produce width 45, clamped to 50
    geom::Rectangle area {
        { 100, 200 },
        { 60,  300 }
    };
    auto container = make_floating(area);
    start_resize(container, mir_resize_edge_east, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 100
            && r.size.width.as_int() == 50;
    }),
                                false))
        .Times(1);

    EXPECT_TRUE(service.handle_pointer_event(185.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, FloatingWindow_WestEdge_MotionRightBeyondMin_ClampsWidthAndResetsX)
{
    // width=60, min=50; dx=+15 would produce width 45, clamped to 50; pos.x must NOT be 115
    geom::Rectangle area {
        { 100, 200 },
        { 60,  300 }
    };
    auto container = make_floating(area);
    start_resize(container, mir_resize_edge_west, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 100 // reset to original, not 115
            && r.size.width.as_int() == 50;
    }),
                                false))
        .Times(1);

    EXPECT_TRUE(service.handle_pointer_event(215.f, 300.f, mir_pointer_action_motion));
}

TEST_F(ResizeServiceTest, FloatingWindow_NorthEdge_MotionDownBeyondMin_ClampsHeightAndResetsY)
{
    // height=60, min=50; dy=+15 would produce height 45, clamped to 50; pos.y must NOT be 215
    geom::Rectangle area {
        { 100, 200 },
        { 400, 60  }
    };
    auto container = make_floating(area);
    start_resize(container, mir_resize_edge_north, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.y.as_int() == 200 // reset to original, not 215
            && r.size.height.as_int() == 50;
    }),
                                false))
        .Times(1);

    EXPECT_TRUE(service.handle_pointer_event(200.f, 315.f, mir_pointer_action_motion));
}

// ---- Floating: drift prevention (multi-step motion) ----

TEST_F(ResizeServiceTest, FloatingWindow_WestEdge_MultipleMotions_AccumulatesWithoutDrift)
{
    // get_logical_area() always returns the original rect even after set_logical_area is called
    // (simulating a compositor that ignores position updates). The service must NOT re-read
    // from the container — it accumulates from its own cached values.
    auto container = make_floating(default_area);
    start_resize(container, mir_resize_edge_west, 200.f, 300.f);

    InSequence seq;
    // First motion: dx=10 → x: 100+10=110, width: 400-10=390
    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 110
            && r.size.width.as_int() == 390;
    }),
                                false))
        .Times(1);
    // Second motion: dx=10 → x: 110+10=120, width: 390-10=380 (accumulated from first result)
    EXPECT_CALL(*container, set_logical_area(Truly([](geom::Rectangle const& r)
    {
        return r.top_left.x.as_int() == 120
            && r.size.width.as_int() == 380;
    }),
                                false))
        .Times(1);

    service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion);
    service.handle_pointer_event(220.f, 300.f, mir_pointer_action_motion);
}

// ---- Tiled window path ----

TEST_F(ResizeServiceTest, TiledWindow_MotionEvent_DoesNotCallSetLogicalAreaAndReturnsTrue)
{
    // With anchored()=true and all neighbors returning nullptr, execute_resize is a no-op.
    // set_logical_area must never be called on the tiled container.
    auto container = make_tiled();
    start_resize(container, mir_resize_edge_east, 200.f, 300.f);

    EXPECT_CALL(*container, set_logical_area(_, _)).Times(0);

    EXPECT_TRUE(service.handle_pointer_event(210.f, 300.f, mir_pointer_action_motion));
}
