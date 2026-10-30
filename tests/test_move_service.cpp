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
