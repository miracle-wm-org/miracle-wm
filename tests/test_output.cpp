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

#include "animator.h"
#include "compositor_state.h"
#include "mock_container.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "output.h"
#include "stub_configuration.h"
#include "workspace_observer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <mir/geometry/rectangle.h>

using namespace miracle;
using ::testing::NiceMock;
using ::testing::Return;

class OutputIntersectTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create mock dependencies
        window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
        animator = std::make_shared<Animator>(nullptr);
        state = std::make_shared<CompositorState>();
        config = std::make_shared<test::StubConfiguration>();

        // Create the Output
        output = std::make_shared<Output>(
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
            animator);

        // Add workspace to the output
        auto registrar = std::make_shared<WorkspaceObserverRegistrar>();
        output->advise_new_workspace(WorkspaceCreationData {
            .id = 1,
            .num = std::nullopt,
            .name = std::string("workspace1"),
            .registrar = registrar });

        // The output creates its own workspace, so we need to get a reference to the actual active workspace
        actual_workspace = output->active();
    }

    std::shared_ptr<Output> output;
    std::shared_ptr<NiceMock<test::MockWindowController>> window_controller;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<test::StubConfiguration> config;
    std::shared_ptr<WorkspaceInterface> actual_workspace;
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
    EXPECT_CALL(*window_controller, get_container(test_window))
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
    EXPECT_CALL(*window_controller, get_container(test_window))
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

TEST_F(OutputIntersectTest, ReturnsShellContainerEvenWhenNotOnActiveWorkspace)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();
    auto different_workspace = std::make_shared<NiceMock<test::MockWorkspace>>();

    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_container(test_window))
        .WillOnce(Return(mock_container));

    // Setup container to be on different workspace but shell type
    EXPECT_CALL(*mock_container, get_workspace())
        .WillOnce(Return(different_workspace));
    EXPECT_CALL(*mock_container, get_type())
        .WillOnce(Return(ContainerType::shell));

    // Act
    auto result = output->intersect(100.0f, 100.0f);

    // Assert
    EXPECT_EQ(result, mock_container);
}

TEST_F(OutputIntersectTest, ReturnsNullWhenContainerNotOnActiveWorkspaceAndNotShell)
{
    // Arrange
    miral::Window test_window;
    auto mock_container = std::make_shared<NiceMock<test::MockContainer>>();
    auto different_workspace = std::make_shared<NiceMock<test::MockWorkspace>>();

    EXPECT_CALL(*window_controller, window_at(100.0f, 100.0f))
        .WillOnce(Return(test_window));
    EXPECT_CALL(*window_controller, get_container(test_window))
        .WillOnce(Return(mock_container));

    // Setup container to be on different workspace and not shell
    EXPECT_CALL(*mock_container, get_workspace())
        .WillOnce(Return(different_workspace));
    EXPECT_CALL(*mock_container, get_type())
        .WillOnce(Return(ContainerType::leaf));

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
    EXPECT_CALL(*window_controller, get_container(test_window))
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
