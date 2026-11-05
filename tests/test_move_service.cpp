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

#include "command_controller.h"
#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_workspace.h"
#include "mode_observer.h"
#include "move_service.h"
#include "output_manager.h"
#include "scratchpad.h"
#include "stub_configuration.h"
#include "stub_container.h"
#include "stub_window_controller.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <deque>
#include <gtest/gtest.h>
#include <mutex>

using namespace miracle;

class StubCommandControllerInterface : public CommandControllerInterface
{
public:
    void quit() override { }
};

class MoveServiceTest : public testing::Test
{

public:
    MoveServiceTest() :
        output_manager(std::make_shared<OutputManager>(std::unique_ptr<test::MockOutputFactory>(output_factory))),
        config(std::make_shared<test::MockConfig>()),
        window_controller(std::make_shared<StubWindowController>(data)),
        workspace_manager(std::make_shared<WorkspaceManager>(workspace_registry, config, output_manager)),
        scratchpad(std::make_shared<Scratchpad>(window_controller, output_manager)),
        command_controller(std::make_shared<CommandController>(
            config,
            state,
            window_controller,
            workspace_manager,
            mode_observer_registrar,
            std::make_unique<StubCommandControllerInterface>(),
            scratchpad,
            output_manager,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr)),
        service(command_controller, config, output_manager)
    {
        ON_CALL(*config, move_modifier())
            .WillByDefault(::testing::Return(mir_input_event_modifier_meta));
        ON_CALL(*config, get_primary_button())
            .WillByDefault(::testing::Return(mir_pointer_button_primary));
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
    MoveService service;

    struct TestOutput
    {
        std::shared_ptr<AbstractOutput> output;
        test::MockOutput* mock;
        std::shared_ptr<::testing::NiceMock<test::MockWorkspace>> workspace;
    };

    /// Creates an output covering [area] whose active workspace has id [workspace_id].
    TestOutput add_output(int id, geom::Rectangle const& area, uint32_t workspace_id)
    {
        auto const& stored_area = areas.emplace_back(area);
        auto* mock = new ::testing::NiceMock<test::MockOutput>();
        auto const workspace = std::make_shared<::testing::NiceMock<test::MockWorkspace>>();
        ON_CALL(*mock, id()).WillByDefault(::testing::Return(id));
        ON_CALL(*mock, get_area()).WillByDefault(::testing::ReturnRef(stored_area));
        ON_CALL(*mock, active()).WillByDefault(::testing::Return(workspace));
        ON_CALL(*mock, get_workspaces())
            .WillByDefault(::testing::Return(std::vector<std::shared_ptr<AbstractWorkspace>> { workspace }));
        ON_CALL(*workspace, id()).WillByDefault(::testing::Return(workspace_id));

        EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(testing::Return(std::shared_ptr<AbstractOutput>(mock)));
        auto const output = output_manager->create("Output" + std::to_string(id), id, area, *workspace_manager);
        // Hold the output weakly, as the output holds the workspace.
        ON_CALL(*workspace, get_output())
            .WillByDefault([weak = std::weak_ptr<AbstractOutput>(output)]
        {
            return weak.lock();
        });
        return { output, mock, workspace };
    }

    /// Makes [container] report [owner] as its output, following it as it is grafted
    /// onto the outputs in [outputs].
    void track_output(
        std::shared_ptr<test::MockContainer> const& container,
        std::shared_ptr<AbstractOutput>& owner,
        std::vector<TestOutput> const& outputs)
    {
        ON_CALL(*container, get_output()).WillByDefault(::testing::ReturnPointee(&owner));
        for (auto const& o : outputs)
        {
            ON_CALL(*o.mock, graft(::testing::_))
                .WillByDefault([&owner, weak = std::weak_ptr<AbstractOutput>(o.output)](auto const&)
            {
                owner = weak.lock();
            });
        }
    }

    std::deque<geom::Rectangle> areas;
};

namespace
{
geom::Rectangle const container_area {
    { 100, 100 },
    { 400, 300 }
};
}

TEST_F(MoveServiceTest, CanMoveMovableContainer)
{
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    test::MockOutput* mock_output = new test::MockOutput();
    ON_CALL(*mock_output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    ON_CALL(*mock_output, get_workspaces())
        .WillByDefault(::testing::Return(workspaces));
    geom::Rectangle const output_area {
        { 0,    0    },
        { 1920, 1080 }
    };
    ON_CALL(*mock_output, get_area())
        .WillByDefault(::testing::ReturnRef(output_area));
    EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::shared_ptr<AbstractOutput>(mock_output)));
    output_manager->create("Output1", 1, output_area, *workspace_manager);

    state->add(container);
    state->focus_container(container);
    ON_CALL(*container, get_logical_area())
        .WillByDefault(::testing::Return(container_area));

    ASSERT_TRUE(service.handle_pointer_event(
        *state,
        150,
        150,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta,
        mir_pointer_button_primary));
    ASSERT_EQ(state->mode(), WindowManagerMode::moving);

    EXPECT_CALL(*container, move_to(150, 120, false));
    EXPECT_TRUE(service.handle_pointer_event(
        *state,
        200,
        170,
        mir_pointer_action_motion,
        mir_input_event_modifier_meta,
        mir_pointer_button_primary));
}

TEST_F(MoveServiceTest, CannotMoveContainerThatIsNotMovable)
{
    // Shell components report that they are not movable.
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    test::MockOutput* mock_output = new test::MockOutput();
    ON_CALL(*mock_output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    ON_CALL(*mock_output, get_workspaces())
        .WillByDefault(::testing::Return(workspaces));
    EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::shared_ptr<AbstractOutput>(mock_output)));
    output_manager->create("Output1", 1, {
                                             { 0,    0    },
                                             { 1920, 1080 }
    },
        *workspace_manager);

    state->add(container);
    state->focus_container(container);
    ON_CALL(*container, is_movable())
        .WillByDefault(::testing::Return(false));

    EXPECT_CALL(*container, move_to(::testing::_, ::testing::_, ::testing::_)).Times(0);

    EXPECT_FALSE(service.handle_pointer_event(
        *state,
        150,
        150,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta,
        mir_pointer_button_primary));
    EXPECT_EQ(state->mode(), WindowManagerMode::normal);
}

TEST_F(MoveServiceTest, MoveStopsWhenFocusMovesToAnotherContainer)
{
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    test::MockOutput* mock_output = new test::MockOutput();
    ON_CALL(*mock_output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    ON_CALL(*mock_output, get_workspaces())
        .WillByDefault(::testing::Return(workspaces));
    EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::shared_ptr<AbstractOutput>(mock_output)));
    output_manager->create("Output1", 1, {
                                             { 0,    0    },
                                             { 1920, 1080 }
    },
        *workspace_manager);

    state->add(container);
    state->focus_container(container);
    ON_CALL(*container, get_logical_area())
        .WillByDefault(::testing::Return(container_area));

    service.handle_pointer_event(
        *state,
        150,
        150,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta,
        mir_pointer_button_primary);
    ASSERT_EQ(state->mode(), WindowManagerMode::moving);

    // Focus moves to another container (e.g. a shell component) mid-move. Neither container
    // may be moved, and the move must end.
    auto const other = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state->add(other);
    state->focus_container(other);

    EXPECT_CALL(*container, move_to(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*other, move_to(::testing::_, ::testing::_, ::testing::_)).Times(0);

    EXPECT_FALSE(service.handle_pointer_event(
        *state,
        200,
        170,
        mir_pointer_action_motion,
        mir_input_event_modifier_meta,
        mir_pointer_button_primary));
    EXPECT_EQ(state->mode(), WindowManagerMode::normal);
}

namespace
{
geom::Rectangle const left_area {
    { 0,    0    },
    { 1920, 1080 }
};
geom::Rectangle const right_area {
    { 1920, 0    },
    { 1920, 1080 }
};
int constexpr buffer = MoveService::output_switch_buffer;
}

class MoveServiceAcrossOutputsTest : public MoveServiceTest
{
public:
    MoveServiceAcrossOutputsTest() :
        left(add_output(1, left_area, 101)),
        right(add_output(2, right_area, 102))
    {
        ON_CALL(*left.mock, intersect(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(container));
        ON_CALL(*container, get_logical_area())
            .WillByDefault(::testing::Return(container_area));
        owner = left.output;
        track_output(container, owner, { left, right });

        state->add(container);
        state->focus_container(container);
    }

    bool press(float x, float y)
    {
        return service.handle_pointer_event(
            *state, x, y, mir_pointer_action_button_down, mir_input_event_modifier_meta, mir_pointer_button_primary);
    }

    bool motion(float x, float y)
    {
        return service.handle_pointer_event(
            *state, x, y, mir_pointer_action_motion, mir_input_event_modifier_meta, mir_pointer_button_primary);
    }

    bool release(float x, float y)
    {
        return service.handle_pointer_event(
            *state, x, y, mir_pointer_action_button_up, mir_input_event_modifier_meta, 0);
    }

    TestOutput left;
    TestOutput right;
    std::shared_ptr<::testing::NiceMock<test::MockContainer>> container
        = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    std::shared_ptr<AbstractOutput> owner;
};

TEST_F(MoveServiceAcrossOutputsTest, ContainerStaysOnOutputWhileCursorIsWithinBuffer)
{
    ASSERT_TRUE(press(1800, 150));

    EXPECT_CALL(*left.mock, delete_container(::testing::_)).Times(0);
    EXPECT_CALL(*right.mock, graft(::testing::_)).Times(0);
    EXPECT_TRUE(motion(1920 + buffer - 1, 150));
    EXPECT_EQ(owner, left.output);
    EXPECT_EQ(state->mode(), WindowManagerMode::moving);
}

TEST_F(MoveServiceAcrossOutputsTest, ContainerTransfersOnceCursorPassesBuffer)
{
    ASSERT_TRUE(press(1800, 150));

    std::shared_ptr<Container> const as_container = container;
    EXPECT_CALL(*left.mock, delete_container(as_container));
    EXPECT_CALL(*right.mock, graft(as_container));
    EXPECT_TRUE(motion(1920 + buffer + 10, 150));

    EXPECT_EQ(owner, right.output);
    EXPECT_EQ(output_manager->focused(), right.output);
    EXPECT_EQ(state->focused_container(), container);
    EXPECT_EQ(state->mode(), WindowManagerMode::moving);
}

TEST_F(MoveServiceAcrossOutputsTest, MoveContinuesAfterTransfer)
{
    ASSERT_TRUE(press(1800, 150));

    // The position is relative to where the move began, regardless of the output.
    int const x = 1920 + buffer + 10;
    EXPECT_CALL(*container, move_to(100 + (x - 1800), 100, false));
    EXPECT_TRUE(motion(x, 150));
    ASSERT_EQ(owner, right.output);

    EXPECT_CALL(*container, move_to(100 + (x + 100 - 1800), 150, false));
    EXPECT_TRUE(motion(x + 100, 200));
}

TEST_F(MoveServiceAcrossOutputsTest, ContainerDoesNotFlapAtBoundary)
{
    ASSERT_TRUE(press(1800, 150));

    EXPECT_CALL(*right.mock, graft(::testing::_)).Times(1);
    EXPECT_CALL(*left.mock, graft(::testing::_)).Times(0);

    EXPECT_TRUE(motion(1920 + buffer + 1, 150));
    for (int i = 0; i < 5; i++)
    {
        EXPECT_TRUE(motion(1920 - buffer + 1, 150));
        EXPECT_TRUE(motion(1920 + buffer - 1, 150));
    }
    EXPECT_EQ(owner, right.output);
}

TEST_F(MoveServiceAcrossOutputsTest, ContainerReturnsOnceCursorPassesBufferOnOriginalOutput)
{
    ASSERT_TRUE(press(1800, 150));
    EXPECT_TRUE(motion(1920 + buffer + 1, 150));
    ASSERT_EQ(owner, right.output);

    EXPECT_CALL(*left.mock, graft(::testing::_));
    EXPECT_TRUE(motion(1920 - buffer - 1, 150));
    EXPECT_EQ(owner, left.output);
}

TEST_F(MoveServiceAcrossOutputsTest, ReleasingWithinBufferTransfersToOutputUnderCursor)
{
    ASSERT_TRUE(press(1800, 150));
    EXPECT_TRUE(motion(1920 + 5, 150));
    ASSERT_EQ(owner, left.output);

    EXPECT_CALL(*right.mock, graft(::testing::_));
    release(1920 + 5, 150);
    EXPECT_EQ(owner, right.output);
    EXPECT_EQ(state->mode(), WindowManagerMode::normal);
}

TEST_F(MoveServiceAcrossOutputsTest, MoveEndsWhenMovabilityIsRevoked)
{
    ASSERT_TRUE(press(1800, 150));

    ON_CALL(*container, is_movable()).WillByDefault(::testing::Return(false));
    EXPECT_CALL(*container, move_to(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*right.mock, graft(::testing::_)).Times(0);

    EXPECT_FALSE(motion(1920 + buffer + 10, 150));
    EXPECT_EQ(state->mode(), WindowManagerMode::normal);
    EXPECT_EQ(owner, left.output);

    // Later motion does not resume the move.
    EXPECT_FALSE(motion(1920 + buffer + 20, 150));
}
