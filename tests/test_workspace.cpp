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

#define GLM_ENABLE_EXPERIMENTAL
#include "compositor_state.h"
#include "leaf_container.h"
#include "mock_container.h"
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
#include <glm/gtx/transform.hpp>
#include <gmock/gmock.h>
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

const geom::Rectangle RESIZED_OUTPUT_SIZE {
    geom::Point(0, 0),
    geom::Size(800, 600)
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
            config,
            window_controller,
            state,
            registry,
            animator,
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
    std::shared_ptr<test::StubConfiguration> config = std::make_shared<test::StubConfiguration>();
    std::shared_ptr<WorkspaceObserverRegistrar> registry = std::make_shared<WorkspaceObserverRegistrar>();
    std::shared_ptr<Animator> animator = std::make_shared<Animator>();
    std::shared_ptr<PluginManager> plugin_manager = make_null_plugin_manager();
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
        plugin_manager);
    auto leaf1 = create_leaf();

    ASSERT_EQ(leaf1->get_workspace(), workspace);
    ASSERT_TRUE(other->add_to_root(*leaf1));
    ASSERT_EQ(leaf1->get_workspace(), other);
    ASSERT_EQ(leaf1->get_logical_area(), OTHER_OUTPUT_SIZE);

    // The container must no longer be part of the workspace that it came from.
    ASSERT_EQ(workspace->get_root()->num_children(), 0);
    ASSERT_TRUE(workspace->is_empty());
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

// ---- floating container (other_containers) lifecycle ----

TEST_F(WorkspaceTest, AddOtherContainer_MakesWorkspaceNonEmpty)
{
    auto container = std::make_shared<NiceMock<test::MockContainer>>();
    EXPECT_TRUE(workspace->is_empty());
    workspace->add_other_container(container, false);
    EXPECT_FALSE(workspace->is_empty());
}

TEST_F(WorkspaceTest, RemoveOtherContainer_EmptiesWorkspaceWhenTilingRootAlsoEmpty)
{
    auto container = std::make_shared<NiceMock<test::MockContainer>>();
    workspace->add_other_container(container, false);
    workspace->remove_other_container(container);
    EXPECT_TRUE(workspace->is_empty());
}

TEST_F(WorkspaceTest, DeleteOtherContainer_NotifiesEmpty)
{
    class Observer : public NullWorkspaceObserver
    {
    public:
        MOCK_METHOD(void, on_workspace_empty, (uint32_t), (override));
    };

    auto const observer = std::make_shared<Observer>();
    registry->register_interest(observer);

    auto container = std::make_shared<NiceMock<test::MockContainer>>();
    workspace->add_other_container(container, false);

    EXPECT_CALL(*observer, on_workspace_empty);
    workspace->delete_container(container);
}

TEST_F(WorkspaceTest, IsEmpty_FalseWhenTiledWindowExistsAlongsideOtherContainer)
{
    auto leaf = create_leaf();
    auto container = std::make_shared<NiceMock<test::MockContainer>>();
    workspace->add_other_container(container, false);
    EXPECT_FALSE(workspace->is_empty());
}

TEST_F(WorkspaceTest, IsEmpty_TrueInitially)
{
    EXPECT_TRUE(workspace->is_empty());
}

TEST_F(WorkspaceTest, ToJson_OtherContainerAppearsInFloatingNodes)
{
    std::string const output_name = "test-output";
    ON_CALL(*output, name()).WillByDefault(ReturnRef(output_name));

    auto container = std::make_shared<NiceMock<test::MockContainer>>();
    ON_CALL(*container, to_json(_)).WillByDefault(Return(nlohmann::json::object()));
    workspace->add_other_container(container, false);

    auto const j = workspace->to_json(false);
    EXPECT_EQ(j["floating_nodes"].size(), 1u);
    EXPECT_TRUE(j["nodes"].empty());
}

TEST_F(WorkspaceTest, RecalculateAreaIsDeferredWhileWorkspaceIsNotShown)
{
    create_leaf();

    // The workspace is not the active workspace on its output.
    ON_CALL(*output, get_area())
        .WillByDefault(ReturnRef(RESIZED_OUTPUT_SIZE));
    workspace->recalculate_area();

    ASSERT_EQ(workspace->get_root()->get_logical_area(), OUTPUT_SIZE);
}

TEST_F(WorkspaceTest, DeferredAreaChangeIsAppliedWhenWorkspaceIsShown)
{
    auto leaf = create_leaf();

    ON_CALL(*output, get_area())
        .WillByDefault(ReturnRef(RESIZED_OUTPUT_SIZE));
    workspace->recalculate_area();

    // The output makes us the active workspace before showing us.
    ON_CALL(*output, active())
        .WillByDefault(Return(workspace));
    workspace->show(geom::Point(0, 0));

    ASSERT_EQ(workspace->get_root()->get_logical_area(), RESIZED_OUTPUT_SIZE);
    ASSERT_EQ(leaf->get_logical_area(), RESIZED_OUTPUT_SIZE);
    ASSERT_EQ(window_controller->get_window_data(leaf).rectangle, leaf->get_visible_area());
}

TEST_F(WorkspaceTest, RecalculateAreaAppliesImmediatelyWhenWorkspaceIsShown)
{
    auto leaf = create_leaf();

    ON_CALL(*output, active())
        .WillByDefault(Return(workspace));
    ON_CALL(*output, get_area())
        .WillByDefault(ReturnRef(RESIZED_OUTPUT_SIZE));
    workspace->recalculate_area();

    ASSERT_EQ(workspace->get_root()->get_logical_area(), RESIZED_OUTPUT_SIZE);
    ASSERT_EQ(window_controller->get_window_data(leaf).rectangle, leaf->get_visible_area());
}

TEST_F(WorkspaceTest, ShowWithAnimationsDisabledResetsAlphaAndTransform)
{
    create_leaf();

    // Simulate the state a workspace is left in after an animated hide:
    // fully transparent and translated offscreen.
    workspace->alpha(0.f);
    workspace->transform(glm::translate(glm::mat4(1.f), glm::vec3(OUTPUT_WIDTH, 0, 0)));

    // Animations are disabled (StubConfiguration), so this takes the instant
    // path and must settle alpha/transform to their final shown values.
    workspace->show(geom::Point(OUTPUT_WIDTH, 0));

    EXPECT_EQ(workspace->alpha(), 1.f);
    EXPECT_EQ(workspace->transform(), glm::mat4(1.f));
}

TEST_F(WorkspaceTest, BeginPreviewRefusesTheActiveWorkspace)
{
    create_leaf();
    ON_CALL(*output, active())
        .WillByDefault(Return(workspace));

    // The active workspace is already in the scene, and showing it again would
    // clobber the state of windows that were never hidden.
    EXPECT_FALSE(workspace->begin_preview());
}

TEST_F(WorkspaceTest, BeginPreviewPutsTheWindowsOfAHiddenWorkspaceBackIntoTheScene)
{
    auto leaf = create_leaf();
    workspace->hide(geom::Point(OUTPUT_WIDTH, 0));
    ASSERT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_hidden);

    EXPECT_TRUE(workspace->begin_preview());
    EXPECT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_restored);
}

TEST_F(WorkspaceTest, BeginPreviewResetsAlphaAndTransform)
{
    create_leaf();

    // The state a workspace is left in after an animated hide: fully
    // transparent and translated offscreen.
    workspace->alpha(0.f);
    workspace->transform(glm::translate(glm::mat4(1.f), glm::vec3(OUTPUT_WIDTH, 0, 0)));

    ASSERT_TRUE(workspace->begin_preview());
    EXPECT_EQ(workspace->alpha(), 1.f);
    EXPECT_EQ(workspace->transform(), glm::mat4(1.f));
}

TEST_F(WorkspaceTest, EndPreviewHidesTheWindowsAgainAndPutsAlphaAndTransformBack)
{
    auto leaf = create_leaf();
    auto const hidden_transform = glm::translate(glm::mat4(1.f), glm::vec3(OUTPUT_WIDTH, 0, 0));
    workspace->hide(geom::Point(OUTPUT_WIDTH, 0));
    workspace->alpha(0.f);
    workspace->transform(hidden_transform);

    ASSERT_TRUE(workspace->begin_preview());
    workspace->end_preview();

    EXPECT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_hidden);
    EXPECT_EQ(workspace->alpha(), 0.f);
    EXPECT_EQ(workspace->transform(), hidden_transform);
}

TEST_F(WorkspaceTest, BeginPreviewIsIdempotent)
{
    create_leaf();

    EXPECT_TRUE(workspace->begin_preview());
    EXPECT_FALSE(workspace->begin_preview());
}

TEST_F(WorkspaceTest, EndPreviewIsANoOpWhenNotPreviewing)
{
    auto leaf = create_leaf();

    workspace->end_preview();
    EXPECT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_restored);
    EXPECT_EQ(workspace->alpha(), 1.f);
}

TEST_F(WorkspaceTest, BeginPreviewAppliesADeferredAreaRecalculation)
{
    auto leaf = create_leaf();

    ON_CALL(*output, get_area())
        .WillByDefault(ReturnRef(RESIZED_OUTPUT_SIZE));
    workspace->recalculate_area();
    ASSERT_EQ(workspace->get_root()->get_logical_area(), OUTPUT_SIZE);

    ASSERT_TRUE(workspace->begin_preview());
    EXPECT_EQ(workspace->get_root()->get_logical_area(), RESIZED_OUTPUT_SIZE);
    EXPECT_EQ(leaf->get_logical_area(), RESIZED_OUTPUT_SIZE);
}

TEST_F(WorkspaceTest, HideWithNoEndPointIsInstantEvenWhenAnimationsAreEnabled)
{
    config->animations_enabled = true;
    auto leaf = create_leaf();

    // An end of (0, 0) means "do not slide anywhere", which is how a caller that
    // has already animated this workspace off screen itself asks to have it put
    // away. The same convention [show] uses for its origin.
    workspace->hide(geom::Point(0, 0));

    EXPECT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_hidden);
}

TEST_F(WorkspaceTest, HideWithAnEndPointAnimatesWhenAnimationsAreEnabled)
{
    config->animations_enabled = true;
    auto leaf = create_leaf();

    // Nothing drives the animator in this fixture, so an animated hide leaves
    // the windows exactly where they are: the containers are only hidden once
    // the animation completes.
    workspace->hide(geom::Point(OUTPUT_WIDTH, 0));

    EXPECT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_restored);
}

TEST_F(WorkspaceTest, EndPreviewLeavesAWorkspaceThatBecameActiveInTheScene)
{
    auto leaf = create_leaf();
    workspace->hide(geom::Point(OUTPUT_WIDTH, 0));
    workspace->alpha(0.f);
    ASSERT_TRUE(workspace->begin_preview());

    // An effect that ends by adopting the workspace it was previewing leaves it
    // as the active one. Putting the preview away must not undo that.
    ON_CALL(*output, active())
        .WillByDefault(Return(workspace));
    workspace->end_preview();

    EXPECT_EQ(window_controller->get_window_data(leaf).state, mir_window_state_restored);
    EXPECT_EQ(workspace->alpha(), 1.f);
}

TEST_F(WorkspaceTest, SelectWindowPrefersTheLastSelectedContainer)
{
    auto const first = create_leaf();
    create_leaf();
    workspace->advise_focus_gained(first);
    window_controller->selected_windows.clear();

    workspace->select_window();

    ASSERT_THAT(window_controller->selected_windows, ElementsAre(first->window().value()));
}

TEST_F(WorkspaceTest, SelectWindowFallsBackToTheFirstWindowWhenNothingWasSelected)
{
    auto const first = create_leaf();
    create_leaf();
    window_controller->selected_windows.clear();

    workspace->select_window();

    ASSERT_THAT(window_controller->selected_windows, ElementsAre(first->window().value()));
}

TEST_F(WorkspaceTest, SelectWindowClearsFocusWhenTheWorkspaceIsEmpty)
{
    window_controller->selected_windows.clear();

    workspace->select_window();

    ASSERT_THAT(window_controller->selected_windows, ElementsAre(miral::Window()));
}
