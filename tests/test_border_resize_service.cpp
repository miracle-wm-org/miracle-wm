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

#include "border_resize_service.h"

#include "command_controller.h"
#include "cursor_override.h"
#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mode_observer.h"
#include "output_manager.h"
#include "resize_service.h"
#include "scratchpad.h"
#include "stub_window_controller.h"
#include "workspace_manager.h"

#include <gtest/gtest.h>
#include <mir/test/doubles/mock_cursor.h>
#include <mir/test/doubles/stub_cursor_image.h>
#include <mir_toolkit/cursors.h>

using namespace miracle;
using namespace testing;

namespace
{
// A 100x100 window with a 10px border. The visible area is the whole window and
// the client content is therefore the inner 80x80 square.
geom::Rectangle const logical {
    { 0,   0   },
    { 100, 100 }
};
geom::Rectangle const visible = logical;
int const border = 10;
}

TEST(BorderEdgeAt, PointInsideTheClientContentIsNotAnEdge)
{
    EXPECT_EQ(mir_resize_edge_none, border_edge_at(logical, visible, border, true, 50, 50));
    EXPECT_EQ(mir_resize_edge_none, border_edge_at(logical, visible, border, false, 50, 50));
}

TEST(BorderEdgeAt, PointOutsideTheLogicalAreaIsNotAnEdge)
{
    EXPECT_EQ(mir_resize_edge_none, border_edge_at(logical, visible, border, true, 150, 50));
    EXPECT_EQ(mir_resize_edge_none, border_edge_at(logical, visible, border, true, 50, -1));
}

TEST(BorderEdgeAt, EachCardinalEdgeIsReported)
{
    EXPECT_EQ(mir_resize_edge_west, border_edge_at(logical, visible, border, true, 2, 50));
    EXPECT_EQ(mir_resize_edge_east, border_edge_at(logical, visible, border, true, 97, 50));
    EXPECT_EQ(mir_resize_edge_north, border_edge_at(logical, visible, border, true, 50, 2));
    EXPECT_EQ(mir_resize_edge_south, border_edge_at(logical, visible, border, true, 50, 97));
}

TEST(BorderEdgeAt, EachCornerIsReported)
{
    EXPECT_EQ(mir_resize_edge_northwest, border_edge_at(logical, visible, border, true, 2, 2));
    EXPECT_EQ(mir_resize_edge_northeast, border_edge_at(logical, visible, border, true, 97, 2));
    EXPECT_EQ(mir_resize_edge_southwest, border_edge_at(logical, visible, border, true, 2, 97));
    EXPECT_EQ(mir_resize_edge_southeast, border_edge_at(logical, visible, border, true, 97, 97));
}

TEST(BorderEdgeAt, EdgeIsPromotedToACornerWithinTheCornerSize)
{
    // Only 12px down the west edge: still inside the 16px corner square.
    ASSERT_LT(12, border_resize_corner_size);
    EXPECT_EQ(mir_resize_edge_northwest, border_edge_at(logical, visible, border, true, 2, 12));

    // ...and 40px down it is a plain west edge.
    EXPECT_EQ(mir_resize_edge_west, border_edge_at(logical, visible, border, true, 2, 40));
}

TEST(BorderEdgeAt, DiagonalsAreSuppressedWhenNotAllowed)
{
    // A tiled container cannot act on a diagonal, so the corner resolves to whichever
    // axis the point is further into the band on.
    EXPECT_EQ(mir_resize_edge_north, border_edge_at(logical, visible, border, false, 8, 2));
    EXPECT_EQ(mir_resize_edge_west, border_edge_at(logical, visible, border, false, 2, 8));
    EXPECT_EQ(mir_resize_edge_west, border_edge_at(logical, visible, border, false, 2, 12));
}

TEST(BorderEdgeAt, TheGapBetweenTheVisibleAndLogicalAreaIsPartOfTheBand)
{
    // A window inset by 5px within its logical area: the gap belongs to the band too.
    geom::Rectangle const gapped_visible {
        { 5,  5  },
        { 90, 90 }
    };
    EXPECT_EQ(mir_resize_edge_west, border_edge_at(logical, gapped_visible, border, true, 1, 50));
    EXPECT_EQ(mir_resize_edge_east, border_edge_at(logical, gapped_visible, border, true, 98, 50));
}

TEST(BorderEdgeAt, AWindowSmallerThanItsBorderIsAllCorner)
{
    geom::Rectangle const tiny {
        { 0, 0 },
        { 8, 8 }
    };
    EXPECT_EQ(mir_resize_edge_northwest, border_edge_at(tiny, tiny, border, true, 4, 4));
    EXPECT_EQ(mir_resize_edge_west, border_edge_at(tiny, tiny, border, false, 4, 4));
}

TEST(CursorNameForEdge, MapsEveryEdgeToItsXdgCursorName)
{
    EXPECT_EQ(mir_vertical_resize_cursor_name, cursor_name_for_edge(mir_resize_edge_north));
    EXPECT_EQ(mir_vertical_resize_cursor_name, cursor_name_for_edge(mir_resize_edge_south));
    EXPECT_EQ(mir_horizontal_resize_cursor_name, cursor_name_for_edge(mir_resize_edge_east));
    EXPECT_EQ(mir_horizontal_resize_cursor_name, cursor_name_for_edge(mir_resize_edge_west));
    EXPECT_EQ(mir_diagonal_resize_top_to_left_cursor_name, cursor_name_for_edge(mir_resize_edge_northwest));
    EXPECT_EQ(mir_diagonal_resize_bottom_to_top_cursor_name, cursor_name_for_edge(mir_resize_edge_northeast));
    EXPECT_EQ(mir_diagonal_resize_bottom_to_left_cursor_name, cursor_name_for_edge(mir_resize_edge_southwest));
    EXPECT_EQ(mir_diagonal_resize_top_to_bottom_cursor_name, cursor_name_for_edge(mir_resize_edge_southeast));
    EXPECT_EQ(mir_default_cursor_name, cursor_name_for_edge(mir_resize_edge_none));
}

namespace
{
class StubCursorOverrideController : public CursorOverrideController
{
public:
    void set_override(std::optional<std::string> const& cursor_name) override
    {
        current = cursor_name;
    }

    std::optional<std::string> current;
};

class StubCommandControllerInterface : public CommandControllerInterface
{
public:
    void quit() override { }
};
}

class BorderResizeServiceTest : public Test
{
public:
    BorderResizeServiceTest() :
        output_manager(std::make_shared<OutputManager>(std::unique_ptr<test::MockOutputFactory>(output_factory))),
        config(std::make_shared<NiceMock<test::MockConfig>>()),
        window_controller(std::make_shared<StubWindowController>(data)),
        workspace_manager(std::make_shared<WorkspaceManager>(workspace_registry, config, output_manager)),
        scratchpad(std::make_shared<Scratchpad>(window_controller, output_manager)),
        command_controller(std::make_shared<CommandController>(
            config, state, window_controller, workspace_manager, mode_observer_registrar,
            std::make_unique<StubCommandControllerInterface>(), scratchpad, output_manager,
            nullptr, nullptr, nullptr, nullptr, nullptr)),
        resize_service(command_controller, config, state, output_manager),
        service(config, state, output_manager, window_controller, cursor_override, resize_service)
    {
        ON_CALL(*config, get_border_config()).WillByDefault(ReturnRef(border_config));
    }

    /// Registers an output whose [intersect] always resolves to \p container.
    void given_output_intersecting(std::shared_ptr<WindowContainer> const& container)
    {
        auto* const mock_output = new NiceMock<test::MockOutput>();
        ON_CALL(*mock_output, intersect(_, _)).WillByDefault(Return(container));
        EXPECT_CALL(*output_factory, create(_, _, _))
            .WillOnce(Return(std::shared_ptr<AbstractOutput>(mock_output)));
        output_manager->create("Output1", 1, {
                                                 { 0,    0    },
                                                 { 1920, 1080 }
        },
            *workspace_manager);
    }

    /// A 100x100 floating window with a 10px border, positioned at the origin.
    std::shared_ptr<NiceMock<test::MockContainer>> given_floating_container()
    {
        auto container = std::make_shared<NiceMock<test::MockContainer>>();
        ON_CALL(*container, get_logical_area()).WillByDefault(Return(logical));
        ON_CALL(*container, get_visible_area()).WillByDefault(Return(visible));
        ON_CALL(*container, anchored()).WillByDefault(Return(false));
        ON_CALL(*container, is_fullscreen()).WillByDefault(Return(false));
        return container;
    }

    std::vector<StubWindowData> data;
    test::MockOutputFactory* output_factory = new test::MockOutputFactory();
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<test::MockConfig> config;
    std::shared_ptr<StubWindowController> window_controller;
    std::shared_ptr<WorkspaceObserverRegistrar> workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<Scratchpad> scratchpad;
    std::shared_ptr<ModeObserverRegistrar> mode_observer_registrar = std::make_shared<ModeObserverRegistrar>();
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<CommandController> command_controller;
    std::shared_ptr<StubCursorOverrideController> cursor_override = std::make_shared<StubCursorOverrideController>();
    BorderConfig border_config { .size = border };
    ResizeService resize_service;
    BorderResizeService service;
};

TEST_F(BorderResizeServiceTest, HoveringABorderSetsTheResizeCursorWithoutConsumingTheEvent)
{
    given_output_intersecting(given_floating_container());

    ASSERT_FALSE(service.handle_pointer_event(2, 50, mir_pointer_action_motion, 0));
    ASSERT_TRUE(cursor_override->current.has_value());
    ASSERT_EQ(mir_horizontal_resize_cursor_name, *cursor_override->current);
}

TEST_F(BorderResizeServiceTest, HoveringTheClientContentReleasesTheOverride)
{
    given_output_intersecting(given_floating_container());

    service.handle_pointer_event(2, 50, mir_pointer_action_motion, 0);
    ASSERT_TRUE(cursor_override->current.has_value());

    ASSERT_FALSE(service.handle_pointer_event(50, 50, mir_pointer_action_motion, 0));
    ASSERT_FALSE(cursor_override->current.has_value());
}

TEST_F(BorderResizeServiceTest, PressingTheBorderStartsAResize)
{
    given_output_intersecting(given_floating_container());

    ASSERT_TRUE(service.handle_pointer_event(2, 50, mir_pointer_action_button_down, mir_pointer_button_primary));
    ASSERT_EQ(WindowManagerMode::resizing, state->mode());
}

TEST_F(BorderResizeServiceTest, PressingTheClientContentDoesNotStartAResize)
{
    given_output_intersecting(given_floating_container());

    ASSERT_FALSE(service.handle_pointer_event(50, 50, mir_pointer_action_button_down, mir_pointer_button_primary));
    ASSERT_EQ(WindowManagerMode::normal, state->mode());
}

TEST_F(BorderResizeServiceTest, AFullscreenWindowHasNoResizeHandles)
{
    auto container = given_floating_container();
    ON_CALL(*container, is_fullscreen()).WillByDefault(Return(true));
    given_output_intersecting(container);

    ASSERT_FALSE(service.handle_pointer_event(2, 50, mir_pointer_action_motion, 0));
    ASSERT_FALSE(cursor_override->current.has_value());
}

TEST_F(BorderResizeServiceTest, ATiledEdgeWithoutANeighborHasNoResizeHandle)
{
    auto container = given_floating_container();
    ON_CALL(*container, anchored()).WillByDefault(Return(true));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(nullptr));
    given_output_intersecting(container);

    ASSERT_FALSE(service.handle_pointer_event(2, 50, mir_pointer_action_motion, 0));
    ASSERT_FALSE(cursor_override->current.has_value());
}

TEST_F(BorderResizeServiceTest, ATiledEdgeWithANeighborHasAResizeHandle)
{
    auto container = given_floating_container();
    auto neighbor = std::make_shared<NiceMock<test::MockContainer>>();
    ON_CALL(*container, anchored()).WillByDefault(Return(true));
    ON_CALL(*container, neighbor_west()).WillByDefault(Return(neighbor));
    given_output_intersecting(container);

    ASSERT_FALSE(service.handle_pointer_event(2, 50, mir_pointer_action_motion, 0));
    ASSERT_TRUE(cursor_override->current.has_value());
    ASSERT_EQ(mir_horizontal_resize_cursor_name, *cursor_override->current);
}

class OverridableCursorTest : public Test
{
public:
    OverridableCursorTest() :
        wrapped(std::make_shared<NiceMock<mir::test::doubles::MockCursor>>()),
        cursor(wrapped)
    {
    }

    std::shared_ptr<NiceMock<mir::test::doubles::MockCursor>> wrapped;
    OverridableCursor cursor;
    std::shared_ptr<mir::graphics::CursorImage> const client_image
        = std::make_shared<mir::test::doubles::StubCursorImage>();
    std::shared_ptr<mir::graphics::CursorImage> const resize_image
        = std::make_shared<mir::test::doubles::StubCursorImage>();
};

TEST_F(OverridableCursorTest, AnOverrideIsShownImmediately)
{
    EXPECT_CALL(*wrapped, show(resize_image));
    cursor.set_override(resize_image);
}

TEST_F(OverridableCursorTest, WhatMirRequestsWhileOverriddenIsNotShown)
{
    cursor.set_override(resize_image);

    EXPECT_CALL(*wrapped, show(_)).Times(0);
    EXPECT_CALL(*wrapped, hide()).Times(0);
    cursor.show(client_image);
    cursor.hide();
}

TEST_F(OverridableCursorTest, ReleasingTheOverrideReplaysTheLastRequest)
{
    cursor.show(client_image);
    cursor.set_override(resize_image);

    EXPECT_CALL(*wrapped, show(client_image));
    cursor.set_override(nullptr);
}

TEST_F(OverridableCursorTest, ReleasingTheOverrideHidesWhenThereWasNothingToReplay)
{
    cursor.set_override(resize_image);

    EXPECT_CALL(*wrapped, hide());
    cursor.set_override(nullptr);
}

TEST_F(OverridableCursorTest, ReleasingTheOverrideFallsBackToTheImageMirHadAlreadyShown)
{
    // Mir shows the default cursor before it hands us the cursor to wrap, so an
    // override released before Mir next asks for an image must restore that one.
    OverridableCursor seeded { wrapped, client_image };
    seeded.set_override(resize_image);

    EXPECT_CALL(*wrapped, show(client_image));
    seeded.set_override(nullptr);
}

TEST_F(OverridableCursorTest, RequestsPassThroughWhileNotOverridden)
{
    EXPECT_CALL(*wrapped, show(client_image));
    cursor.show(client_image);
}
