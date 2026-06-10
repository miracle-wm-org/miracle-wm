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
#include "container_listener.h"
#include "freestyle_window_container.h"
#include "mock_configuration.h"
#include "mock_output.h"
#include "mock_session.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
geom::Rectangle const output_area {
    { 0,    0    },
    { 1920, 1080 }
};
geom::Point const window_position { 100, 200 };
geom::Size const window_size { 400, 300 };
int const border_size = 4;
}

class FreestyleWindowContainerTest : public ::testing::Test
{
public:
    FreestyleWindowContainerTest() :
        session(std::make_shared<NiceMock<test::MockSession>>()),
        surface(std::make_shared<NiceMock<test::MockSurface>>()),
        app(session),
        window(app, surface)
    {
        ON_CALL(*workspace, get_output()).WillByDefault(Return(output));
        ON_CALL(*output, get_area()).WillByDefault(ReturnRef(output_area));
        ON_CALL(*output, get_transform()).WillByDefault(Return(glm::mat4(1.f)));
        ON_CALL(*workspace, transform()).WillByDefault(Return(glm::mat4(1.f)));
        ON_CALL(*config, get_border_config()).WillByDefault(ReturnRef(border_config));
        ON_CALL(*surface, top_left()).WillByDefault(Return(window_position));
        ON_CALL(*surface, window_size()).WillByDefault(Return(window_size));

        container_with_border = std::make_shared<FreestyleWindowContainer>(
            window, window_controller, state, workspace, config, true);
        container_without_border = std::make_shared<FreestyleWindowContainer>(
            window, window_controller, state, workspace, config, false);

        state->add(container_with_border);
        state->add(container_without_border);
    }

protected:
    BorderConfig border_config { .size = border_size };
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
    std::shared_ptr<NiceMock<test::MockConfig>> config = std::make_shared<NiceMock<test::MockConfig>>();
    std::shared_ptr<NiceMock<test::MockWorkspace>> workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    std::shared_ptr<NiceMock<test::MockOutput>> output = std::make_shared<NiceMock<test::MockOutput>>();
    std::shared_ptr<NiceMock<test::MockSession>> session;
    std::shared_ptr<NiceMock<test::MockSurface>> surface;
    miral::Application app;
    miral::Window window;
    std::shared_ptr<FreestyleWindowContainer> container_with_border;
    std::shared_ptr<FreestyleWindowContainer> container_without_border;
};

// ---- needs_outline ----

TEST_F(FreestyleWindowContainerTest, NeedsOutlineIsTrueWhenHasBorder)
{
    EXPECT_TRUE(container_with_border->needs_outline());
}

TEST_F(FreestyleWindowContainerTest, NeedsOutlineIsFalseWhenNoBorder)
{
    EXPECT_FALSE(container_without_border->needs_outline());
}

// ---- anchored ----

TEST_F(FreestyleWindowContainerTest, AnchoredReturnsFalse)
{
    EXPECT_FALSE(container_with_border->anchored());
    EXPECT_FALSE(container_without_border->anchored());
}

// ---- fullscreen ----

TEST_F(FreestyleWindowContainerTest, IsFullscreenReturnsFalseByDefault)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    EXPECT_FALSE(container_with_border->is_fullscreen());
}

TEST_F(FreestyleWindowContainerTest, IsFullscreenReturnsTrueWhenStateIsFullscreen)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_TRUE(container_with_border->is_fullscreen());
}

TEST_F(FreestyleWindowContainerTest, ToggleFullscreenReturnsTrueAndCallsChangeState)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_fullscreen)).Times(1);
    EXPECT_TRUE(container_with_border->toggle_fullscreen());
}

TEST_F(FreestyleWindowContainerTest, ToggleFullscreenWhenAlreadyFullscreenRestores)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    EXPECT_TRUE(container_with_border->toggle_fullscreen());
}

TEST_F(FreestyleWindowContainerTest, ToggleFullscreenNotifiesObserver)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_fullscreen, (Container const&), (override));
    };

    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    auto listener = std::make_shared<Listener>();
    container_with_border->register_interest(listener);
    EXPECT_CALL(*listener, on_container_fullscreen(_)).Times(1);
    container_with_border->toggle_fullscreen();
}

TEST_F(FreestyleWindowContainerTest, ToggleFullscreenRestoresAreaWhenLeavingFullscreen)
{
    // First call: window at (100,200) 400x300 → goes fullscreen, saves area
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    container_with_border->toggle_fullscreen();

    // Second call: window is now fullscreen → restore; set_rectangle should use saved area
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& r)
    {
        return r.top_left == geom::Point { 100, 200 }
        && r.size == geom::Size { 400, 300 };
    }),
                                        _))
        .Times(1);
    container_with_border->toggle_fullscreen();
}

TEST_F(FreestyleWindowContainerTest, ConstrainCallsNoclipWhenFullscreen)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_CALL(*window_controller, noclip(window)).Times(1);
    container_with_border->constrain();
}

TEST_F(FreestyleWindowContainerTest, HandleModifyWithStateChangeCallsChangeState)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_fullscreen)).Times(1);
    container_with_border->handle_modify(spec);
}

// ---- workspace ----

TEST_F(FreestyleWindowContainerTest, GetWorkspaceReturnsConstructedWorkspace)
{
    EXPECT_EQ(container_with_border->get_workspace(), workspace);
}

TEST_F(FreestyleWindowContainerTest, SetWorkspaceUpdatesWorkspace)
{
    auto new_workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    container_with_border->set_workspace(new_workspace);
    EXPECT_EQ(container_with_border->get_workspace(), new_workspace);
}

TEST_F(FreestyleWindowContainerTest, SetWorkspaceNotifiesObserver)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_workspace_changed, (Container const&), (override));
    };

    auto listener = std::make_shared<Listener>();
    container_with_border->register_interest(listener);
    EXPECT_CALL(*listener, on_container_workspace_changed(_)).Times(1);

    auto new_workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    container_with_border->set_workspace(new_workspace);
}

// ---- constrain ----

TEST_F(FreestyleWindowContainerTest, ConstrainWithBorderNeverclips)
{
    EXPECT_CALL(*window_controller, noclip(_)).Times(1);
    container_with_border->constrain();
}

// ---- visible area ----

TEST_F(FreestyleWindowContainerTest, GetVisibleAreaWithoutBorderMatchesLogicalArea)
{
    auto const logical = container_without_border->get_logical_area();
    auto const visible = container_without_border->get_visible_area();
    EXPECT_EQ(logical, visible);
}

// ---- move_to ----

TEST_F(FreestyleWindowContainerTest, MoveToCallsModifyWithCorrectTopLeft)
{
    EXPECT_CALL(*window_controller, modify(window, Truly([](miral::WindowSpecification const& spec)
    {
        return spec.top_left().is_set()
            && spec.top_left().value() == geom::Point { 50, 75 };
    }))).Times(1);

    container_with_border->move_to(50, 75, false);
}

// ---- move_by ----

TEST_F(FreestyleWindowContainerTest, MoveByRightIncreasesX)
{
    // window is at (100, 200), moving right by 30 → (130, 200)
    EXPECT_CALL(*window_controller, modify(window, Truly([](miral::WindowSpecification const& spec)
    {
        return spec.top_left().is_set()
            && spec.top_left().value().x.as_int() == 130
            && spec.top_left().value().y.as_int() == 200;
    }))).Times(AtLeast(1));

    container_with_border->move_by(Direction::right, 30);
}

TEST_F(FreestyleWindowContainerTest, MoveByLeftDecreasesX)
{
    // window is at (100, 200), moving left by 30 → (70, 200)
    EXPECT_CALL(*window_controller, modify(window, Truly([](miral::WindowSpecification const& spec)
    {
        return spec.top_left().is_set()
            && spec.top_left().value().x.as_int() == 70
            && spec.top_left().value().y.as_int() == 200;
    }))).Times(AtLeast(1));

    container_with_border->move_by(Direction::left, 30);
}

TEST_F(FreestyleWindowContainerTest, MoveByDownIncreasesY)
{
    // window is at (100, 200), moving down by 20 → (100, 220)
    EXPECT_CALL(*window_controller, modify(window, Truly([](miral::WindowSpecification const& spec)
    {
        return spec.top_left().is_set()
            && spec.top_left().value().x.as_int() == 100
            && spec.top_left().value().y.as_int() == 220;
    }))).Times(AtLeast(1));

    container_with_border->move_by(Direction::down, 20);
}

TEST_F(FreestyleWindowContainerTest, MoveByUpDecreasesY)
{
    // window is at (100, 200), moving up by 20 → (100, 180)
    EXPECT_CALL(*window_controller, modify(window, Truly([](miral::WindowSpecification const& spec)
    {
        return spec.top_left().is_set()
            && spec.top_left().value().x.as_int() == 100
            && spec.top_left().value().y.as_int() == 180;
    }))).Times(AtLeast(1));

    container_with_border->move_by(Direction::up, 20);
}

// ---- resize ----

TEST_F(FreestyleWindowContainerTest, ResizeRightIncreasesWidth)
{
    // window is 400x300. Resize right by 50 → width becomes 450, position unchanged
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& rect)
    {
        return rect.top_left.x.as_int() == 100
            && rect.top_left.y.as_int() == 200
            && rect.size.width.as_int() == 450
            && rect.size.height.as_int() == 300;
    }),
                                        _))
        .Times(1);

    container_with_border->resize(Direction::right, 50);
}

TEST_F(FreestyleWindowContainerTest, ResizeLeftDecreasesWidth)
{
    // Resize left by 50 → width decreases by 50 to 350, position unchanged
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& rect)
    {
        return rect.top_left.x.as_int() == 100
            && rect.top_left.y.as_int() == 200
            && rect.size.width.as_int() == 350
            && rect.size.height.as_int() == 300;
    }),
                                        _))
        .Times(1);

    container_with_border->resize(Direction::left, 50);
}

TEST_F(FreestyleWindowContainerTest, ResizeDownIncreasesHeight)
{
    // Resize down by 50 → height becomes 350, position unchanged
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& rect)
    {
        return rect.top_left.x.as_int() == 100
            && rect.top_left.y.as_int() == 200
            && rect.size.width.as_int() == 400
            && rect.size.height.as_int() == 350;
    }),
                                        _))
        .Times(1);

    container_with_border->resize(Direction::down, 50);
}

TEST_F(FreestyleWindowContainerTest, ResizeUpDecreasesHeight)
{
    // Resize up by 50 → height decreases by 50 to 250, position unchanged
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& rect)
    {
        return rect.top_left.x.as_int() == 100
            && rect.top_left.y.as_int() == 200
            && rect.size.width.as_int() == 400
            && rect.size.height.as_int() == 250;
    }),
                                        _))
        .Times(1);

    container_with_border->resize(Direction::up, 50);
}

// ---- set_size ----

TEST_F(FreestyleWindowContainerTest, SetSizeCallsSetRectangleWithNewDimensions)
{
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& rect)
    {
        return rect.size.width.as_int() == 600
            && rect.size.height.as_int() == 400;
    }),
                                        _))
        .Times(1);

    container_with_border->set_size(600, 400);
}

TEST_F(FreestyleWindowContainerTest, SetSizeWithNulloptPreservesCurrentDimension)
{
    // With no width specified, width should stay at 400 (current window_size width)
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& rect)
    {
        return rect.size.width.as_int() == 400
            && rect.size.height.as_int() == 500;
    }),
                                        _))
        .Times(1);

    container_with_border->set_size(std::nullopt, 500);
}

// ---- drag ----

TEST_F(FreestyleWindowContainerTest, DragStartReturnsTrueFirstTime)
{
    EXPECT_TRUE(container_with_border->drag_start());
}

TEST_F(FreestyleWindowContainerTest, DragStartReturnsFalseWhenAlreadyDragging)
{
    container_with_border->drag_start();
    EXPECT_FALSE(container_with_border->drag_start());
}

TEST_F(FreestyleWindowContainerTest, DragMovesWindow)
{
    container_with_border->drag_start();

    EXPECT_CALL(*window_controller, modify(window, Truly([](miral::WindowSpecification const& spec)
    {
        return spec.top_left().is_set()
            && spec.top_left().value() == geom::Point { 150, 250 };
    }))).Times(1);

    container_with_border->drag(150, 250);
}

TEST_F(FreestyleWindowContainerTest, DragStopReturnsTrueWhenDragging)
{
    container_with_border->drag_start();
    EXPECT_TRUE(container_with_border->drag_stop());
}

TEST_F(FreestyleWindowContainerTest, DragStopReturnsFalseWhenNotDragging)
{
    EXPECT_FALSE(container_with_border->drag_stop());
}

TEST_F(FreestyleWindowContainerTest, DragStopNotifiesObserver)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_moved, (Container const&), (override));
    };

    auto listener = std::make_shared<Listener>();
    container_with_border->register_interest(listener);
    EXPECT_CALL(*listener, on_container_moved(_)).Times(1);

    container_with_border->drag_start();
    container_with_border->drag(200, 300);
    container_with_border->drag_stop();
}

// ---- maximize ----

TEST_F(FreestyleWindowContainerTest, MoveToWhenMaximizedRestoresState)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_maximized));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_with_border->move_to(500, 600, false);
}

TEST_F(FreestyleWindowContainerTest, MoveByWhenMaximizedRestoresState)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_maximized));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_with_border->move_by(Direction::right, 30);
}

TEST_F(FreestyleWindowContainerTest, ResizeWhenMaximizedRestoresState)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_maximized));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_with_border->resize(Direction::right, 50);
}

TEST_F(FreestyleWindowContainerTest, SetSizeWhenMaximizedRestoresState)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_maximized));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_with_border->set_size(800, 600);
}

TEST_F(FreestyleWindowContainerTest, DragStartWhenMaximizedRestoresState)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_maximized));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    EXPECT_TRUE(container_with_border->drag_start());
}

// ---- to_json ----

TEST_F(FreestyleWindowContainerTest, ToJsonHasCorrectTypeForWaybarCompatibility)
{
    miral::WindowInfo info(window, {});
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    ON_CALL(*window_controller, info_for(window))
        .WillByDefault(ReturnRef(info));
    auto const j = container_with_border->to_json(false);
    EXPECT_EQ(j["type"], "floating_con");
}
