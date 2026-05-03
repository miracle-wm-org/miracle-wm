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

#include "animator.h"
#include "compositor_state.h"
#include "mock_container.h"
#include "mock_shell_application_spawner.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "output.h"
#include "shell_application_manager.h"
#include "stub_configuration.h"
#include "workspace_observer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <mir/geometry/rectangle.h>

using namespace miracle;
using ::testing::NiceMock;
using ::testing::Return;

class OutputTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        // Create mock dependencies
        window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
        animator = std::make_shared<Animator>();
        state = std::make_shared<CompositorState>();
        config = std::make_shared<test::StubConfiguration>();
        shell_application_manager = std::make_shared<ShellApplicationManager>(
            std::make_unique<NiceMock<test::MockShellApplicationSpawner>>());
    }

    std::shared_ptr<NiceMock<test::MockWindowController>> window_controller;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<test::StubConfiguration> config;
    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::shared_ptr<AbstractWorkspace> actual_workspace;
};

class OutputIntersectTest : public OutputTest
{
protected:
    void SetUp() override
    {
        OutputTest::SetUp();

        output = std::make_shared<Output>(
            shell_application_manager,
            "TestOutput",
            1, // id
            geom::Rectangle {
                { 0,    0    },
                { 1920, 1080 }
        }, // area
            OutputConfigDetails {}, // output_config
            state,
            config,
            window_controller,
            animator,
            std::make_shared<PluginManager>());

        auto const registrar = std::make_shared<WorkspaceObserverRegistrar>();
        output->advise_new_workspace(WorkspaceCreationData {
            .id = 1,
            .num = std::nullopt,
            .name = std::string("workspace1"),
            .registrar = registrar });
        actual_workspace = output->active();
    }

    std::shared_ptr<Output> output;
};

TEST_F(OutputIntersectTest, ReturnsNullWhenNoWindowAtPosition)
{
    // Arrange
    miral::Window empty_window;
    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(empty_window));

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, nullptr);
}

TEST_F(OutputIntersectTest, ReturnsNullWhenWindowControllerReturnsNoContainer)
{
    // Arrange
    miral::Window test_window;
    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_window_container(test_window))
        .WillOnce(Return(nullptr));

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, nullptr);
}

TEST_F(OutputIntersectTest, ReturnsContainerWhenOnActiveWorkspace)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();

    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_window_container(test_window))
        .WillOnce(Return(mock_container));

    // Setup container to be on active workspace
    auto active_workspace = output->active();
    EXPECT_CALL(*mock_container, get_workspace())
        .WillOnce(Return(active_workspace));
    // get_type() may or may not be called due to short-circuit evaluation

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, mock_container);
}

TEST_F(OutputIntersectTest, ReturnsNullWhenOnContainerIsAnimating)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();

    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_window_container(test_window))
        .WillOnce(Return(mock_container));

    // Setup container to be on active workspace and append an animation to the
    // animator so that it is animating.
    animator->append(Animation(
        1,
        AnimationDefinition(),
        AnimationData(),
        [](auto const&) { },
        std::make_shared<PluginManager>()));
    EXPECT_CALL(*mock_container, animation_handle())
        .WillRepeatedly(Return(1));

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, nullptr);
}

TEST_F(OutputIntersectTest, ReturnsNullWhenContainerOnDifferentWorkspace)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();
    auto different_workspace = std::make_shared<NiceMock<test::MockWorkspace>>();

    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_window_container(test_window))
        .WillOnce(Return(mock_container));

    // Setup container to be on different workspace
    EXPECT_CALL(*mock_container, get_workspace())
        .WillOnce(Return(different_workspace));

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, nullptr);
}

TEST_F(OutputIntersectTest, ReturnsNullWhenContainerNotOnActiveWorkspaceAndNotShell)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();
    auto different_workspace = std::make_shared<NiceMock<test::MockWorkspace>>();

    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_window_container(test_window))
        .WillOnce(Return(mock_container));

    // Setup container to be on different workspace
    EXPECT_CALL(*mock_container, get_workspace())
        .WillOnce(Return(different_workspace));

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, nullptr);
}

TEST_F(OutputIntersectTest, HandlesDifferentCoordinates)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();

    float test_x = 500.5f;
    float test_y = 300.7f;

    EXPECT_CALL(*window_controller, window_at(test_x, test_y))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_window_container(test_window))
        .WillOnce(Return(mock_container));

    // Get the active workspace and set container to be on it
    auto active_workspace = output->active();
    EXPECT_CALL(*mock_container, get_workspace())
        .WillOnce(Return(active_workspace));
    // get_type() may or may not be called due to short-circuit evaluation

    // Act
    auto result = output->intersect(test_x, test_y);

    // Assert
    EXPECT_EQ(result, mock_container);
}

TEST_F(OutputTest, OutputToJsonWithUnsetCurrentMode)
{
    auto const output = std::make_shared<Output>(
        shell_application_manager,
        "TestOutput",
        1, // id
        geom::Rectangle {
            { 0,    0    },
            { 1920, 1080 }
    },
        OutputConfigDetails {},
        state,
        config,
        window_controller,
        animator,
        std::make_shared<PluginManager>());
    EXPECT_THAT(output->get_outputs_json(false)["current_mode"], testing::Eq(nlohmann::json({
                                                                     { "width",   0 },
                                                                     { "height",  0 },
                                                                     { "refresh", 0 }
    })));
    EXPECT_THAT(output->to_json(false)["current_mode"], testing::Eq(nlohmann::json({
                                                            { "width",   0 },
                                                            { "height",  0 },
                                                            { "refresh", 0 }
    })));
}

TEST_F(OutputTest, OutputToJsonWithInvalidCurrentMode)
{
    OutputConfigDetails details;
    details.current_mode_index = 10000;
    auto const output = std::make_shared<Output>(
        shell_application_manager,
        "TestOutput",
        1, // id
        geom::Rectangle {
            { 0,    0    },
            { 1920, 1080 }
    },
        details,
        state,
        config,
        window_controller,
        animator,
        std::make_shared<PluginManager>());
    EXPECT_THAT(output->get_outputs_json(false)["current_mode"], testing::Eq(nlohmann::json({
                                                                     { "width",   0 },
                                                                     { "height",  0 },
                                                                     { "refresh", 0 }
    })));
    EXPECT_THAT(output->to_json(false)["current_mode"], testing::Eq(nlohmann::json({
                                                            { "width",   0 },
                                                            { "height",  0 },
                                                            { "refresh", 0 }
    })));
}