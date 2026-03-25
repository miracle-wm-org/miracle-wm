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
#include "leaf_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_shell_application_spawner.h"
#include "output_manager.h"
#include "parent_container.h"
#include "passthrough_server_action_queue.h"
#include "shell_application_manager.h"
#include "stub_configuration.h"
#include "stub_surface.h"
#include "stub_window_controller.h"
#include "window_controller.h"
#include "workspace.h"
#include "workspace_observer.h"
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
const int OUTPUT_WIDTH = 1280;
const int OUTPUT_HEIGHT = 720;

const geom::Rectangle OUTPUT_SIZE {
    geom::Point(0, 0),
    geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT)
};

const geom::Rectangle OTHER_OUTPUT_SIZE {
    geom::Point(OUTPUT_WIDTH, OUTPUT_HEIGHT),
    geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT)
};

std::vector<std::shared_ptr<AbstractWorkspace>> empty_workspaces;
std::vector<miral::Zone> empty_app_zones;

std::shared_ptr<test::MockOutput> create_output(geom::Rectangle const& bounds)
{
    auto output = std::make_shared<NiceMock<test::MockOutput>>();
    ON_CALL(*output, get_area())
        .WillByDefault(ReturnRef(bounds));
    ON_CALL(*output, get_workspaces())
        .WillByDefault(Return(empty_workspaces));
    ON_CALL(*output, get_app_zones())
        .WillByDefault(ReturnRef(empty_app_zones));
    return output;
}
}

class WorkspaceTest : public Test
{
public:
    WorkspaceTest() :
        state(std::make_shared<CompositorState>()),
        output(create_output(OUTPUT_SIZE)),
        window_controller(std::make_shared<StubWindowController>(pairs)),
        shell_application_manager(std::make_shared<ShellApplicationManager>(
            std::make_unique<NiceMock<test::MockShellApplicationSpawner>>())),
        workspace(std::make_shared<Workspace>(
            shell_application_manager,
            output,
            0,
            0,
            "0",
            std::make_shared<test::StubConfiguration>(),
            window_controller,
            state,
            registry,
            animator,
            std::make_shared<PassthroughServerActionQueue>(),
            plugin_manager))
    {
    }

    std::shared_ptr<LeafContainer> create_leaf(
        std::optional<std::shared_ptr<ParentContainer>> parent = std::nullopt,
        AbstractWorkspace* target_workspace = nullptr)
    {
        if (target_workspace == nullptr)
            target_workspace = workspace.get();
        miral::WindowSpecification spec;
        miral::ApplicationInfo app_info;
        auto const layout_parent = target_workspace->get_layout_container();
        spec = layout_parent->place_new_window(spec, std::nullopt);

        auto const surface = std::make_shared<test::StubSurface>();
        surfaces.push_back(surface);

        miral::Window const window(nullptr, surface);
        miral::WindowInfo const info(window, spec);
        auto leaf = layout_parent->confirm_window(window);
        pairs.push_back({ window, leaf, geom::Rectangle(), mir_window_state_restored, std::nullopt });

        state->add(std::dynamic_pointer_cast<WindowContainer>(leaf));
        leaf->on_focus_gained();
        state->focus_container(leaf);
        return Container::as_leaf(leaf);
    }

    std::shared_ptr<CompositorState> state;
    std::vector<std::shared_ptr<test::StubSurface>> surfaces;
    std::vector<StubWindowData> pairs;
    std::shared_ptr<test::MockOutput> output;
    std::shared_ptr<StubWindowController> window_controller;
    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::shared_ptr<WorkspaceObserverRegistrar> registry = std::make_shared<WorkspaceObserverRegistrar>();
    std::shared_ptr<Animator> animator = std::make_shared<Animator>();
    std::shared_ptr<PluginManager> plugin_manager = std::make_shared<PluginManager>();
    std::shared_ptr<Workspace> workspace;
};

TEST_F(WorkspaceTest, CanAddSingleWindowWithoutBorderAndGaps)
{
    auto leaf = create_leaf();
    ASSERT_EQ(leaf->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf->get_logical_area().top_left, geom::Point(0, 0));
}

TEST_F(WorkspaceTest, CanAddTwoWindowsHorizontallyWithoutBorderAndGaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(OUTPUT_WIDTH / 2.f, 0));
}

TEST_F(WorkspaceTest, CanAddTwoWindowsVerticallyWithoutBorderAndGaps)
{
    auto leaf1 = create_leaf();
    leaf1->request_vertical_layout();

    auto leaf2 = create_leaf();
    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT / 2.f));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT / 2.f));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, OUTPUT_HEIGHT / 2.f));
}

TEST_F(WorkspaceTest, CanAddThreeWindowsHorizontallyWithoutBorderAndGaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    auto leaf3 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(ceilf(OUTPUT_WIDTH / 3.f), OUTPUT_HEIGHT));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(ceilf(OUTPUT_WIDTH / 3.f), OUTPUT_HEIGHT));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(ceilf(OUTPUT_WIDTH / 3.f), 0));

    ASSERT_EQ(leaf3->get_logical_area().size, geom::Size(floorf(OUTPUT_WIDTH / 3.f), OUTPUT_HEIGHT));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(floorf(OUTPUT_WIDTH * (2.f / 3.f)) + 1, 0));
}

TEST_F(WorkspaceTest, CanStartDraggingALeaf)
{
    auto leaf1 = create_leaf();
    ASSERT_TRUE(leaf1->drag_start());
}

TEST_F(WorkspaceTest, CanDragALeafToAPosition)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(50, 50);
    auto const& data = window_controller->get_window_data(leaf1);
    ASSERT_EQ(data.rectangle.top_left.x.as_int(), 50);
    ASSERT_EQ(data.rectangle.top_left.y.as_int(), 50);
}

TEST_F(WorkspaceTest, CanStopDraggingALeaf)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(50, 50);
    leaf1->drag_stop();
    auto const& data = window_controller->get_window_data(leaf1);
    ASSERT_EQ(data.rectangle.top_left.x.as_int(), 0);
    ASSERT_EQ(data.rectangle.top_left.y.as_int(), 0);
}

TEST_F(WorkspaceTest, CanMoveContainerToSibling)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_TRUE(leaf1->move_to(*leaf2));

    // Assert that leaf2 is in the first position
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 0));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(OUTPUT_WIDTH / 2.f, 0));
}

TEST_F(WorkspaceTest, CanMoveContainerToDifferentParent)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    leaf2->request_vertical_layout();
    auto leaf3 = create_leaf(leaf2->get_parent().lock());

    ASSERT_TRUE(leaf1->move_to(*leaf3));

    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 0));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(0, ceilf(OUTPUT_HEIGHT / 3.f)));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, ceilf(OUTPUT_HEIGHT * (2.f / 3.f))));
    ASSERT_EQ(workspace->get_root()->num_children(), 3);
}

TEST_F(WorkspaceTest, CanMoveContainerToContainerInOtherTree)
{
    auto other_output = create_output(OTHER_OUTPUT_SIZE);
    auto const other = std::make_shared<Workspace>(
        shell_application_manager,
        other_output,
        1,
        1,
        "1",
        std::make_shared<test::StubConfiguration>(),
        window_controller,
        state,
        registry,
        animator,
        std::make_shared<PassthroughServerActionQueue>(),
        plugin_manager);
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf(std::nullopt, other.get());

    ASSERT_EQ(leaf1->get_workspace(), workspace);
    ASSERT_EQ(leaf2->get_workspace(), other);

    ASSERT_TRUE(leaf1->move_to(*leaf2));

    ASSERT_EQ(leaf2->get_workspace(), other);
}

TEST_F(WorkspaceTest, CanMoveContainerToTree)
{
    auto const other_output = create_output(OTHER_OUTPUT_SIZE);
    auto other = std::make_shared<Workspace>(
        shell_application_manager,
        other_output,
        1,
        1,
        "1",
        std::make_shared<test::StubConfiguration>(),
        window_controller,
        state,
        registry,
        animator,
        std::make_shared<PassthroughServerActionQueue>(),
        plugin_manager);
    auto leaf1 = create_leaf();

    ASSERT_EQ(leaf1->get_workspace(), workspace);
    ASSERT_TRUE(other->add_to_root(*leaf1));
    ASSERT_EQ(leaf1->get_workspace(), other);
    ASSERT_EQ(leaf1->get_logical_area(), OTHER_OUTPUT_SIZE);
}

TEST_F(WorkspaceTest, DraggedWindowsDoNotChangeTheirPositionWhenANewWindowIsAdded)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(100, 100);

    auto leaf2 = create_leaf();
    ASSERT_EQ(window_controller->get_window_data(leaf1).rectangle.top_left, mir::geometry::Point(100, 100));
}

TEST_F(WorkspaceTest, DraggedWindowsAreUnconstrained)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    ASSERT_EQ(window_controller->get_window_data(leaf1).clip, std::nullopt);
    leaf1->drag(100, 100);
    leaf1->drag_stop();
    ASSERT_EQ(window_controller->get_window_data(leaf1).clip, leaf1->get_visible_area());
}

TEST_F(WorkspaceTest, WorkspaceBoundsAreInitializedToOutputSizeWhenNoAppZonesArePresent)
{
    // Assert that the first tree (w/o app zones) is equal to the output size.
    ASSERT_EQ(workspace->get_root()->get_logical_area(), OUTPUT_SIZE);
}

TEST_F(WorkspaceTest, WorkspaceBoundsAreInitializedToFirstZoneSizeWhenAppZonesArePresent)
{
    auto output = std::make_shared<NiceMock<test::MockOutput>>();
    ON_CALL(*output, get_area())
        .WillByDefault(ReturnRef(OTHER_OUTPUT_SIZE));
    ON_CALL(*output, get_workspaces())
        .WillByDefault(Return(empty_workspaces));

    mir::geometry::Rectangle const zone_bounds(
        mir::geometry::Point(100, 100),
        mir::geometry::Size(500, 500));
    std::vector<miral::Zone> zones = { miral::Zone(zone_bounds) };
    ON_CALL(*output, get_app_zones())
        .WillByDefault(ReturnRef(zones));
    auto const other = std::make_shared<Workspace>(
        shell_application_manager,
        output,
        1,
        1,
        "1",
        std::make_shared<test::StubConfiguration>(),
        window_controller,
        state,
        registry,
        animator,
        std::make_shared<PassthroughServerActionQueue>(),
        plugin_manager);

    // Assert that the first tree (w/o app zones) is equal to the output size.
    ASSERT_EQ(other->get_root()->get_logical_area(), zone_bounds);
}

TEST_F(WorkspaceTest, GetWorkspaceJson)
{
    std::string const output_name = "test";
    EXPECT_CALL(*output, name)
        .WillOnce(ReturnRef(output_name));
    EXPECT_CALL(*output, active)
        .WillOnce(Return(workspace));

    auto const json = workspace->get_workspaces_json(true);
    EXPECT_THAT(json["num"], Eq(0));
    EXPECT_THAT(json["name"], Eq("0:0"));
    EXPECT_THAT(json["visible"], Eq(true));
    EXPECT_THAT(json["focused"], Eq(true));
    EXPECT_THAT(json["urgent"], Eq(false));
    EXPECT_THAT(json["output"], Eq("test"));
    EXPECT_THAT(json["rect"]["x"], Eq(0));
    EXPECT_THAT(json["rect"]["y"], Eq(0));
    EXPECT_THAT(json["rect"]["width"], Eq(OUTPUT_WIDTH));
    EXPECT_THAT(json["rect"]["height"], Eq(OUTPUT_HEIGHT));
}

TEST_F(WorkspaceTest, CanSetNum)
{
    workspace->num(2);
    EXPECT_THAT(workspace->num(), Eq(2));
}

TEST_F(WorkspaceTest, CanSetName)
{
    workspace->name("meow");
    EXPECT_THAT(workspace->name(), Eq("meow"));
}

TEST_F(WorkspaceTest, NotifiesWhenEmpty)
{
    class Observer : public NullWorkspaceObserver
    {
    public:
        MOCK_METHOD(void, on_workspace_empty, (uint32_t), (override));
    };

    auto const observer = std::make_shared<Observer>();
    registry->register_interest(observer);
    auto const leaf = create_leaf();
    EXPECT_CALL(*observer, on_workspace_empty);
    workspace->delete_container(leaf);
}
