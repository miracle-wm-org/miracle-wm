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

#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_parent_container.h"
#include "mock_workspace.h"
#include "mode_observer.h"
#include "resize_service.h"
#include "scratchpad.h"
#include "stub_window_controller.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <gtest/gtest.h>
#include <mutex>

using namespace miracle;

class StubCommandControllerInterface final : public CommandControllerInterface
{
public:
    void quit() override { }
};

class ResizeServiceTest : public testing::Test
{
public:
    ResizeServiceTest() :
        output_manager(std::make_shared<OutputManager>(std::unique_ptr<test::MockOutputFactory>(output_factory))),
        config(std::make_shared<testing::NiceMock<test::MockConfig>>()),
        window_controller(std::make_shared<StubWindowController>(data)),
        workspace_manager(std::make_shared<WorkspaceManager>(workspace_registry, config, output_manager)),
        scratchpad(std::make_shared<Scratchpad>(window_controller, output_manager)),
        command_controller(std::make_shared<CommandController>(
            config,
            mutex,
            state,
            window_controller,
            workspace_manager,
            mode_observer_registrar,
            std::make_unique<StubCommandControllerInterface>(),
            scratchpad,
            output_manager)),
        service(command_controller, config, state, output_manager)
    {
    }

    std::recursive_mutex mutex;
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
    ResizeService service;
};

TEST_F(ResizeServiceTest, CanStartResizing)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east);

    ASSERT_EQ(state->mode(), WindowManagerMode::resizing);
    ASSERT_TRUE(service.handle_pointer_event(100, 100, mir_pointer_action_button_down, 0));
}

TEST_F(ResizeServiceTest, CannotResizeWhenParentHasMoreThanOneChild)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(2));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east);

    ASSERT_EQ(state->mode(), WindowManagerMode::normal);
}

TEST_F(ResizeServiceTest, CannotResizeAnchoredContainer)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(true));

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east);

    ASSERT_EQ(state->mode(), WindowManagerMode::normal);
}

TEST_F(ResizeServiceTest, CannotResizeNonLeafContainer)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east);

    ASSERT_EQ(state->mode(), WindowManagerMode::normal);
}

TEST_F(ResizeServiceTest, ResizeNorthEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_north);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(100, 80), geom::Size(200, 220)), false));
    ASSERT_TRUE(service.handle_pointer_event(150, 80, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeSouthEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_south);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 250)), false));
    ASSERT_TRUE(service.handle_pointer_event(150, 350, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeEastEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(100, 100), geom::Size(250, 200)), false));
    ASSERT_TRUE(service.handle_pointer_event(350, 150, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeWestEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_west);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(80, 100), geom::Size(220, 200)), false));
    ASSERT_TRUE(service.handle_pointer_event(80, 150, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeNorthEastEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_northeast);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(100, 80), geom::Size(150, 220)), false));
    ASSERT_TRUE(service.handle_pointer_event(250, 80, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeNorthWestEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_northwest);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(80, 80), geom::Size(220, 220)), false));
    ASSERT_TRUE(service.handle_pointer_event(80, 80, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeSouthEastEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_southeast);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(100, 100), geom::Size(150, 150)), false));
    ASSERT_TRUE(service.handle_pointer_event(250, 250, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, ResizeSouthWestEdge)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200)), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));
    ON_CALL(*parent, get_area())
        .WillByDefault(::testing::Return(geom::Rectangle(geom::Point(100, 100), geom::Size(200, 200))));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_southwest);
    EXPECT_CALL(*parent, set_logical_area(geom::Rectangle(geom::Point(80, 100), geom::Size(220, 150)), false));
    ASSERT_TRUE(service.handle_pointer_event(80, 250, mir_pointer_action_motion, 0));
}

TEST_F(ResizeServiceTest, StopsResizingWhenButtonReleased)
{
    auto const container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    auto const parent = std::make_shared<::testing::NiceMock<test::MockParentContainer>>(
        state, window_controller, config, geom::Rectangle(), nullptr, nullptr, false);

    ON_CALL(*container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container, get_parent())
        .WillByDefault(::testing::Return(parent));
    ON_CALL(*container, anchored())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*parent, get_parent())
        .WillByDefault(::testing::Return(std::weak_ptr<ParentContainer>()));

    state->add(container);
    state->focus_container(container);

    service.handle_request_resize(container, mir_pointer_action_button_down, mir_resize_edge_east);

    ASSERT_TRUE(service.handle_pointer_event(100, 100, mir_pointer_action_button_down, 0));
    ASSERT_FALSE(service.handle_pointer_event(100, 100, mir_pointer_action_button_up, 0));
    ASSERT_EQ(state->mode(), WindowManagerMode::normal);
}
