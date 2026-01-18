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
#include "drag_and_drop_service.h"
#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "mode_observer.h"
#include "output_manager.h"
#include "scratchpad.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

using namespace miracle;
using namespace testing;

class StubCommandControllerInterface : public CommandControllerInterface
{
public:
    void quit() override { }
};

std::unique_ptr<NiceMock<test::MockOutputFactory>> create_output_factory(std::shared_ptr<OutputInterface> const& output)
{
    auto output_factory = std::make_unique<NiceMock<test::MockOutputFactory>>();
    ON_CALL(*output_factory, create)
        .WillByDefault(Return(output));
    return output_factory;
}

class CommandControllerTest : public Test
{
public:
    CommandControllerTest() :
        output_manager(std::make_shared<OutputManager>(create_output_factory(output))),
        config(std::make_shared<NiceMock<test::MockConfig>>()),
        window_controller(std::make_shared<NiceMock<test::MockWindowController>>()),
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
            output_manager))
    {
        Mock::AllowLeak(output.get());
    }

    std::shared_ptr<NiceMock<test::MockOutput>> output = std::make_shared<NiceMock<test::MockOutput>>();
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<test::MockConfig> config;
    std::shared_ptr<test::MockWindowController> window_controller;
    std::shared_ptr<WorkspaceObserverRegistrar> workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<Scratchpad> scratchpad;
    std::shared_ptr<ModeObserverRegistrar> mode_observer_registrar = std::make_shared<ModeObserverRegistrar>();
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<CommandController> command_controller;
};

MATCHER_P(GapsEq, expected, "")
{
    return arg == expected;
}

TEST_F(CommandControllerTest, SetInnerGapsSetsGlobalGaps)
{
    size_t constexpr TEST_GAP = 10;
    auto const gaps = Gaps { TEST_GAP, TEST_GAP, TEST_GAP, TEST_GAP };
    EXPECT_CALL(*config, override_inner_gaps(GapsEq(gaps)));
    command_controller->set_inner_gaps(TEST_GAP, GapsChangeType::set, false);
}

TEST_F(CommandControllerTest, SetInnerGapsAddsToGlobalGaps)
{
    size_t constexpr TEST_GAP = 5;
    Gaps constexpr initial_gaps { 10, 10, 10, 10 };
    EXPECT_CALL(*config, get_inner_gaps()).WillOnce(Return(initial_gaps));
    auto constexpr result = Gaps { 15, 15, 15, 15 };
    EXPECT_CALL(*config, override_inner_gaps(GapsEq(result))).Times(1);
    command_controller->set_inner_gaps(TEST_GAP, GapsChangeType::plus, false);
}

TEST_F(CommandControllerTest, SetInnerGapsSubtractsFromGlobalGaps)
{
    size_t constexpr TEST_GAP = 3;
    Gaps constexpr initial_gaps { 10, 10, 10, 10 };
    EXPECT_CALL(*config, get_inner_gaps()).WillOnce(Return(initial_gaps));

    auto constexpr result = Gaps { 7, 7, 7, 7 };
    EXPECT_CALL(*config, override_inner_gaps(GapsEq(result))).Times(1);
    command_controller->set_inner_gaps(TEST_GAP, GapsChangeType::minus, false);
}

TEST_F(CommandControllerTest, SetInnerGapsSetsWorkspaceGaps)
{
    size_t constexpr TEST_GAP = 8;
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    auto const workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    Mock::AllowLeak(workspace.get());
    workspaces.push_back(workspace);

    EXPECT_CALL(*workspace, get_output()).WillRepeatedly(Return(output));
    EXPECT_CALL(*output, get_workspaces()).WillRepeatedly(ReturnRef(workspaces));
    EXPECT_CALL(*output, active()).WillRepeatedly(Return(workspace));

    output_manager->create("test", 1, geom::Rectangle({ 0, 0 }, { 1280, 920 }), *workspace_manager);
    output_manager->focus(output->id());

    auto constexpr result = Gaps { TEST_GAP, TEST_GAP, TEST_GAP, TEST_GAP };
    EXPECT_CALL(*workspace, inner_gaps(GapsEq(result)));
    command_controller->set_inner_gaps(TEST_GAP, GapsChangeType::set, true);
}

TEST_F(CommandControllerTest, SetOuterGapsSetsGlobalGaps)
{
    size_t constexpr TEST_GAP = 10;
    auto constexpr result = Gaps { TEST_GAP, TEST_GAP, TEST_GAP, TEST_GAP };
    EXPECT_CALL(*config, override_outer_gaps(GapsEq(result)));
    command_controller->set_outer_gaps(TEST_GAP, OuterGapsChange::outer, GapsChangeType::set, false);
}

TEST_F(CommandControllerTest, SetOuterGapsSetsHorizontalGaps)
{
    size_t constexpr TEST_GAP = 5;
    Gaps constexpr initial_gaps { 10, 10, 10, 10 };
    EXPECT_CALL(*config, get_outer_gaps()).WillOnce(Return(initial_gaps));
    auto constexpr result = Gaps { 10, 10, 5, 5 };
    EXPECT_CALL(*config, override_outer_gaps(GapsEq(result))).Times(1);
    command_controller->set_outer_gaps(TEST_GAP, OuterGapsChange::horizontal, GapsChangeType::set, false);
}

TEST_F(CommandControllerTest, SetOuterGapsAddsToVerticalGaps)
{
    size_t constexpr TEST_GAP = 3;
    Gaps constexpr initial_gaps { 10, 10, 10, 10 };
    EXPECT_CALL(*config, get_outer_gaps()).WillOnce(Return(initial_gaps));
    auto constexpr result = Gaps { 13, 13, 10, 10 };
    EXPECT_CALL(*config, override_outer_gaps(GapsEq(result))).Times(1);
    command_controller->set_outer_gaps(TEST_GAP, OuterGapsChange::vertical, GapsChangeType::plus, false);
}

TEST_F(CommandControllerTest, SetOuterGapsSetsWorkspaceGaps)
{
    size_t constexpr TEST_GAP = 8;
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    auto const workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    Mock::AllowLeak(workspace.get());
    workspaces.push_back(workspace);

    EXPECT_CALL(*workspace, get_output()).WillRepeatedly(Return(output));
    EXPECT_CALL(*output, get_workspaces()).WillRepeatedly(ReturnRef(workspaces));
    EXPECT_CALL(*output, active()).WillRepeatedly(Return(workspace));

    output_manager->create("test", 1, geom::Rectangle({ 0, 0 }, { 1280, 920 }), *workspace_manager);
    output_manager->focus(output->id());

    auto constexpr result = Gaps { TEST_GAP, TEST_GAP, TEST_GAP, TEST_GAP };
    EXPECT_CALL(*workspace, outer_gaps(GapsEq(result)));
    command_controller->set_outer_gaps(TEST_GAP, OuterGapsChange::outer, GapsChangeType::set, true);
}

TEST_F(CommandControllerTest, CannotMoveActiveToSameWorkspaceByNumber)
{
    auto const container = std::make_shared<NiceMock<test::MockContainer>>();
    state->add(container);
    state->focus_container(container);

    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    EXPECT_CALL(*output, get_workspaces())
        .WillRepeatedly(ReturnRef(workspaces));

    output_manager->create("hello", 1, geom::Rectangle({ 0, 0 }, { 1280, 920 }), *workspace_manager);
    output_manager->focus(output->id());

    auto const workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    EXPECT_CALL(*container, get_workspace())
        .WillOnce(Return(workspace));
    EXPECT_CALL(*workspace, num())
        .WillOnce(Return(1));

    ASSERT_TRUE(command_controller->try_move_to_workspace({}, 1, true));
}

TEST_F(CommandControllerTest, CannotMoveActiveToSameWorkspaceByName)
{
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    EXPECT_CALL(*output, get_workspaces)
        .WillRepeatedly(ReturnRef(workspaces));
    output_manager->create("hello", 1, geom::Rectangle({ 0, 0 }, { 1280, 920 }), *workspace_manager);

    auto const container = std::make_shared<NiceMock<test::MockContainer>>();
    state->add(container);
    state->focus_container(container);

    auto const workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    EXPECT_CALL(*container, get_workspace())
        .WillOnce(Return(workspace));
    std::optional<std::string> const name = "Test";
    EXPECT_CALL(*workspace, name())
        .WillOnce(ReturnRef(name));

    std::string expected = "Test";
    ASSERT_FALSE(command_controller->try_move_to_workspace_named({}, expected, false));
}

TEST_F(CommandControllerTest, CanGetAllMarks)
{
    auto const container1 = std::make_shared<NiceMock<test::MockContainer>>();
    auto const first_result = std::vector<std::string> { "a", "b", "c" };
    state->add(container1);
    state->focus_container(container1);
    EXPECT_CALL(*container1, get_marks())
        .WillOnce(ReturnRef(first_result));

    auto const container2 = std::make_shared<NiceMock<test::MockContainer>>();
    state->add(container2);
    state->focus_container(container2);
    auto const second_result = std::vector<std::string> { "a", "d", "e" };
    EXPECT_CALL(*container2, get_marks())
        .WillOnce(ReturnRef(second_result));

    auto const result = command_controller->get_all_marks();
    std::unordered_set<std::string> expected = { "a", "b", "c", "d", "e" };
    EXPECT_THAT(result, Eq(expected));
}

TEST_F(CommandControllerTest, CanRenameSelectedWorkspace)
{
    // Setup: Add an output to the output manager and mock a workspace such
    // that it is associated with the output
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    EXPECT_CALL(*output, get_workspaces)
        .WillRepeatedly(ReturnRef(workspaces));
    output_manager->create("hello", 1, geom::Rectangle({ 0, 0 }, { 1280, 920 }), *workspace_manager);

    auto const workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    Mock::AllowLeak(workspace.get());
    EXPECT_CALL(*workspace, id)
        .WillRepeatedly(Return(5));
    EXPECT_CALL(*workspace, num())
        .WillOnce(Return(1));
    std::optional<std::string> name = "hello";
    EXPECT_CALL(*workspace, name())
        .WillOnce(ReturnRef(name));
    EXPECT_CALL(*workspace, get_output())
        .WillRepeatedly(Return(output));
    EXPECT_CALL(*output, active())
        .WillRepeatedly(Return(workspace));
    workspaces.push_back(workspace);

    // Act: Rename the workspace and assert that it is getting the new name
    std::optional<std::string> const new_name = "hi";
    std::optional<int> constexpr new_num = 2;
    EXPECT_CALL(*workspace, num(new_num));
    EXPECT_CALL(*workspace, name(new_name));
    command_controller->rename_selected_workspace({ .number = 2,
        .name = "hi" });
}

TEST_F(CommandControllerTest, CanRenameExistingWorkspace)
{
    // Setup: Add an output to the output manager and mock a workspace such
    // that it is associated with the output
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    EXPECT_CALL(*output, get_workspaces)
        .WillRepeatedly(ReturnRef(workspaces));
    output_manager->create("hello", 1, geom::Rectangle({ 0, 0 }, { 1280, 920 }), *workspace_manager);

    auto const workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    Mock::AllowLeak(workspace.get());
    EXPECT_CALL(*workspace, id)
        .WillRepeatedly(Return(5));
    EXPECT_CALL(*workspace, num())
        .WillRepeatedly(Return(1));
    std::optional<std::string> name = "hello";
    EXPECT_CALL(*workspace, name())
        .WillRepeatedly(ReturnRef(name));
    EXPECT_CALL(*workspace, get_output())
        .WillRepeatedly(Return(output));
    EXPECT_CALL(*output, active())
        .WillRepeatedly(Return(nullptr));
    workspaces.push_back(workspace);

    // Act: Rename the workspace and assert that it is getting the new name
    std::optional<std::string> const new_name = "hi";
    std::optional<int> constexpr new_num = 2;
    EXPECT_CALL(*workspace, num(new_num));
    EXPECT_CALL(*workspace, name(new_name));
    command_controller->rename_existing_workspace(
        { .number = 1,
            .name = "hello" },
        { .number = 2,
            .name = "hi" });
}

TEST_F(CommandControllerTest, CannotResizeWhileNotInNormalOrResizingState)
{
    state->mode(WindowManagerMode::moving);

    auto const container = std::make_shared<NiceMock<test::MockContainer>>();
    state->add(container);
    state->focus_container(container);

    command_controller->try_toggle_resize_mode();
    EXPECT_THAT(state->mode(), Eq(WindowManagerMode::moving));
}

TEST_F(CommandControllerTest, CanToggleResizeModeToResizing)
{
    state->mode(WindowManagerMode::normal);

    auto const container = std::make_shared<NiceMock<test::MockContainer>>();
    EXPECT_CALL(*container, get_type())
        .WillOnce(Return(ContainerType::regular));
    state->add(container);
    state->focus_container(container);

    command_controller->try_toggle_resize_mode();
    EXPECT_THAT(state->mode(), Eq(WindowManagerMode::resizing));
}

TEST_F(CommandControllerTest, CanToggleResizeModeToNormal)
{
    state->mode(WindowManagerMode::resizing);

    auto const container = std::make_shared<NiceMock<test::MockContainer>>();
    EXPECT_CALL(*container, get_type())
        .WillOnce(Return(ContainerType::regular));
    state->add(container);
    state->focus_container(container);

    command_controller->try_toggle_resize_mode();
    EXPECT_THAT(state->mode(), Eq(WindowManagerMode::normal));
}
