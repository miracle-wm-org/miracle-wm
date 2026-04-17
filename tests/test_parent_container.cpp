/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include "mock_container.h"
#include "mock_output_factory.h"
#include "mock_session.h"
#include "mock_shell_application_spawner.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "output_manager.h"
#include "parent_container.h"
#include "shell_application_manager.h"
#include "stub_configuration.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <miral/application.h>
#include <miral/window.h>

using namespace miracle;
using namespace testing;
namespace geom = mir::geometry;

class ParentContainerData
{
public:
    ParentContainerData(
        std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<Config> const& config,
        geom::Rectangle const& area) :
        parent(std::make_shared<ParentContainer>(shell_application_manager, state, window_controller, config, area, workspace, nullptr))
    {
    }

    std::shared_ptr<test::MockWorkspace> workspace = std::make_shared<test::MockWorkspace>();
    geom::Rectangle area;
    std::shared_ptr<ParentContainer> parent;
};

class ParentContainerTest : public Test
{
public:
    [[nodiscard]] ParentContainerData make_parent(geom::Rectangle const& area) const
    {
        return ParentContainerData { shell_application_manager, state, window_controller, config, area };
    }

    std::shared_ptr<ShellApplicationManager> shell_application_manager = std::make_shared<ShellApplicationManager>(
        std::make_unique<NiceMock<test::MockShellApplicationSpawner>>());
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<testing::NiceMock<test::MockWindowController>>();
    std::shared_ptr<Config> config = std::make_shared<test::StubConfiguration>();
    std::shared_ptr<test::MockWorkspace> workspace = std::make_shared<test::MockWorkspace>();
};

TEST_F(ParentContainerTest, WhenParentReceivesFocusThenChildrenAreRaised)
{
    // Arrange
    ParentContainerData const parent_data = make_parent(geom::Rectangle({ 0, 0 }, { 800, 800 }));
    std::shared_ptr<test::MockContainer> const child1 = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> const child2 = std::make_shared<NiceMock<test::MockContainer>>();
    parent_data.parent->add_child(child1, 0);
    parent_data.parent->add_child(child2, 1);

    auto const session = std::make_shared<testing::NiceMock<test::MockSession>>();
    auto const surface = std::make_shared<testing::NiceMock<test::MockSurface>>();
    miral::Application const app = session;
    miral::Window window(app, surface);

    ON_CALL(*child1, window())
        .WillByDefault(Return(window));
    ON_CALL(*child2, window())
        .WillByDefault(Return(window));

    // We're allowing the leak because GMock is mad for a reason that
    // we don't currently care much about
    Mock::AllowLeak(child1.get());
    Mock::AllowLeak(child2.get());

    // Expect
    EXPECT_CALL(*window_controller, raise).Times(2);

    // Act
    parent_data.parent->on_focus_gained();
}

class ParentContainerSwapTest : public ParentContainerTest
{
public:
    ParentContainerSwapTest()
    {
        first_parent_data.parent->add_child(container1, 0);
        second_parent_data.parent->add_child(container2, 0);
        second_parent_data.parent->add_child(container3, 1);
    }

    ParentContainerData const first_parent_data = make_parent(geom::Rectangle({ 0, 0 }, { 800, 800 }));
    ParentContainerData const second_parent_data = make_parent(geom::Rectangle({ 800, 0 }, { 1200, 800 }));
    std::shared_ptr<test::MockContainer> const container1 = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> const container2 = std::make_shared<NiceMock<test::MockContainer>>();
    std::shared_ptr<test::MockContainer> const container3 = std::make_shared<NiceMock<test::MockContainer>>();
};

TEST_F(ParentContainerSwapTest, SwapContainersBetweenParentsSetsParents)
{
    // We're allowing the leak because GMock is mad for a reason that
    // we don't currently care much about
    Mock::AllowLeak(container1.get());
    Mock::AllowLeak(container2.get());
    Mock::AllowLeak(container3.get());

    // Act: Swap the containers on different parent
    EXPECT_CALL(*container1, set_parent(Eq(second_parent_data.parent)));
    EXPECT_CALL(*container2, set_parent(Eq(first_parent_data.parent)));

    ParentContainer::swap(
        first_parent_data.parent,
        0,
        second_parent_data.parent,
        0);
}

TEST_F(ParentContainerSwapTest, SwapContainersBetweenParentsSetsWorkspaces)
{
    // We're allowing the leak because GMock is mad for a reason that
    // we don't currently care much about
    Mock::AllowLeak(container1.get());
    Mock::AllowLeak(container2.get());
    Mock::AllowLeak(container3.get());

    // Act: Swap the containers on different parent
    EXPECT_CALL(*container1, set_workspace(Eq(second_parent_data.workspace)));
    EXPECT_CALL(*container2, set_workspace(Eq(first_parent_data.workspace)));

    ParentContainer::swap(
        first_parent_data.parent,
        0,
        second_parent_data.parent,
        0);
}

TEST_F(ParentContainerSwapTest, SwapContainersBetweenParentsSetsLogicalArea)
{
    // We're allowing the leak because GMock is mad for a reason that
    // we don't currently care much about
    Mock::AllowLeak(container1.get());
    Mock::AllowLeak(container2.get());
    Mock::AllowLeak(container3.get());

    // Act: Swap the containers on different parent
    geom::Rectangle first_rectangle({ 0, 0 }, { 800, 800 });
    geom::Rectangle second_rectangle({ 800, 0 }, { 600, 800 });
    EXPECT_CALL(*container1, get_logical_area)
        .WillOnce(Return(first_rectangle));
    EXPECT_CALL(*container2, get_logical_area)
        .WillOnce(Return(second_rectangle));
    EXPECT_CALL(*container1, set_logical_area(Eq(second_rectangle), testing::_));
    EXPECT_CALL(*container2, set_logical_area(Eq(first_rectangle), testing::_));

    ParentContainer::swap(
        first_parent_data.parent,
        0,
        second_parent_data.parent,
        0);
}

TEST_F(ParentContainerSwapTest, SwapContainersBetweenParentsOnDifferentWorkspacesCausesTransformChange)
{
    // We're allowing the leak because GMock is mad for a reason that
    // we don't currently care much about
    Mock::AllowLeak(container1.get());
    Mock::AllowLeak(container2.get());
    Mock::AllowLeak(container3.get());

    // Act: Swap the containers on different parent
    EXPECT_CALL(*container1, set_workspace(testing::_));
    EXPECT_CALL(*container2, set_workspace(testing::_));

    ParentContainer::swap(
        first_parent_data.parent,
        0,
        second_parent_data.parent,
        0);
}
