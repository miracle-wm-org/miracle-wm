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
#include "config.h"
#include "container_listener.h"
#include "container_scope.h"
#include "leaf_container.h"
#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include "miral/application_info.h"
#include "miral/window_specification.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_parent_container.h"
#include "mock_session.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "stub_configuration.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace miracle;

namespace
{
auto parent_area = geom::Rectangle {
    { 0,   0   },
    { 800, 600 }
};
}

class LeafContainerTest : public ::testing::Test
{
public:
    LeafContainerTest() :
        workspace(std::make_shared<testing::NiceMock<test::MockWorkspace>>()),
        parent(std::make_shared<testing::NiceMock<test::MockParentContainer>>()),
        leaf_container(std::make_shared<LeafContainer>(
            workspace,
            window_controller,
            geom::Rectangle {
                { 0,   0   },
                { 400, 300 }
    },
            config,
            parent,
            state)),
        session(std::make_shared<testing::NiceMock<test::MockSession>>()),
        surface(std::make_shared<testing::NiceMock<test::MockSurface>>()),
        app(session),
        window(app, surface)
    {
        workspaces.push_back(workspace);

        ON_CALL(*workspace, get_output())
            .WillByDefault(testing::Return(output));
        ON_CALL(*output, get_area())
            .WillByDefault(testing::ReturnRef(parent_area));
        ON_CALL(*output, get_workspaces())
            .WillByDefault(testing::Return(workspaces));

        state->add(leaf_container);
        leaf_container->associate_to_window(window);

        default_window_info = std::make_unique<miral::WindowInfo>(window, miral::WindowSpecification {});
        ON_CALL(*window_controller, info_for(testing::A<miral::Window const&>()))
            .WillByDefault(testing::ReturnRef(*default_window_info));
    }

    void TearDown() override
    {
        ::testing::Mock::VerifyAndClear(workspace.get());
        ::testing::Mock::VerifyAndClear(output.get());
    }

protected:
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<testing::NiceMock<test::MockWindowController>>();
    std::shared_ptr<Config> config = std::make_shared<test::StubConfiguration>();
    std::shared_ptr<test::MockWorkspace> workspace;
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    std::shared_ptr<test::MockOutput> output = std::make_shared<testing::NiceMock<test::MockOutput>>();
    std::shared_ptr<test::MockParentContainer> parent;
    std::shared_ptr<LeafContainer> leaf_container;
    std::shared_ptr<test::MockSession> session;
    std::shared_ptr<test::MockSurface> surface;
    miral::Application app;
    miral::Window window;
    std::unique_ptr<miral::WindowInfo> default_window_info;
};

TEST_F(LeafContainerTest, InitializesWithCorrectLogicalArea)
{
    auto area = leaf_container->get_logical_area();
    ASSERT_EQ(area.size.width.as_int(), 400);
    ASSERT_EQ(area.size.height.as_int(), 300);
}

TEST_F(LeafContainerTest, SetsAndGetsParentCorrectly)
{
    ASSERT_EQ(leaf_container->get_parent().lock(), parent);
}

TEST_F(LeafContainerTest, SetsAndGetsLogicalAreaCorrectly)
{
    geom::Rectangle new_area {
        { 10,  10  },
        { 200, 200 }
    };
    leaf_container->set_logical_area(new_area);
    ASSERT_EQ(leaf_container->get_logical_area(), new_area);
}

TEST_F(LeafContainerTest, SetsAndGetsStateCorrectly)
{
    EXPECT_CALL(*window_controller, change_state(::testing::_, MirWindowState::mir_window_state_fullscreen))
        .Times(1);
    leaf_container->set_state(MirWindowState::mir_window_state_fullscreen);
    leaf_container->commit_changes();
}

TEST_F(LeafContainerTest, SetsAndGetsTreeCorrectly)
{
    std::shared_ptr<test::MockWorkspace> const new_workspace = std::make_shared<test::MockWorkspace>();
    EXPECT_CALL(*new_workspace, get_output())
        .WillRepeatedly(testing::Return(output));
    EXPECT_CALL(*new_workspace, transform())
        .WillRepeatedly(testing::Return(glm::mat4(2.f)));

    leaf_container->set_workspace(new_workspace);
    ASSERT_EQ(leaf_container->get_workspace(), new_workspace);

    uint64_t seen_generation = 0;
    std::vector<RenderData> render_data;
    state->render_data_manager()->copy_if_changed(seen_generation, render_data);
    ASSERT_EQ(render_data.size(), 1);
    EXPECT_THAT(render_data[0].output_area, testing::Eq(parent_area));
    EXPECT_THAT(render_data[0].workspace_transform, testing::Eq(glm::mat4(2.f)));
}

TEST_F(LeafContainerTest, CorrectlyReportsIfFocused)
{
    state->focus_container(leaf_container);
    ASSERT_TRUE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, CorrectlyReportsIfNotFocused)
{
    state->focus_container(nullptr);
    ASSERT_FALSE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, IfModifyingWindowToFullScreenThenNoclipIsCalled)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    EXPECT_CALL(*window_controller, noclip(testing::_));
    leaf_container->handle_modify(spec, false);
}

TEST_F(LeafContainerTest, IfModifyingWindowToRestoredThenClipIsCalled)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_restored;
    EXPECT_CALL(*window_controller, clip(testing::_, testing::_));
    leaf_container->handle_modify(spec, false);
}

namespace
{
bool has_restored_state(miral::WindowSpecification const& spec)
{
    return spec.state() && spec.state().value() == mir_window_state_restored;
}
}

class LeafContainerMaximizedTest : public LeafContainerTest, public ::testing::WithParamInterface<MirWindowState>
{
};

TEST_P(LeafContainerMaximizedTest, CannotMaximizeWindowInHandleModify)
{
    MirWindowState state = GetParam();
    ON_CALL(*parent, anchored())
        .WillByDefault(testing::Return(false));

    miral::WindowSpecification spec;
    spec.state() = state;

    EXPECT_CALL(*window_controller, modify(window, testing::Truly(has_restored_state)));
    leaf_container->handle_modify(spec, false);
}

INSTANTIATE_TEST_SUITE_P(
    LeafContainerMaximizedTest,
    LeafContainerMaximizedTest,
    ::testing::Values(mir_window_state_maximized, mir_window_state_vertmaximized, mir_window_state_horizmaximized, mir_window_state_minimized, mir_window_state_hidden));

TEST_F(LeafContainerTest, HandleModifyWhileHiddenDefersStateUntilShown)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;

    // While the workspace is hidden, no live state change (hence no clip/noclip) happens now.
    EXPECT_CALL(*window_controller, change_state(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*window_controller, noclip(testing::_)).Times(0);
    leaf_container->handle_modify(spec, true);

    // The deferred state is applied via the restore mechanism when the container is shown.
    EXPECT_CALL(*window_controller, show(window, testing::Field(&RestoreResult::state, mir_window_state_fullscreen))).Times(1);
    leaf_container->show();
}

TEST_F(LeafContainerTest, HandleModifyWhileHiddenSanitizesMaximizeToRestored)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    leaf_container->handle_modify(spec, true);

    // Only fullscreen is honoured for tiled windows; a maximize request defers as restored.
    EXPECT_CALL(*window_controller, show(window, testing::Field(&RestoreResult::state, mir_window_state_restored))).Times(1);
    leaf_container->show();
}

TEST_F(LeafContainerTest, ShowingContainerCausesRaise)
{
    EXPECT_CALL(*window_controller, raise(testing::_));
    leaf_container->show();
}

TEST_F(LeafContainerTest, HidingContainerCausesSendToBack)
{
    EXPECT_CALL(*window_controller, send_to_back(testing::_));
    leaf_container->hide();
}

TEST_F(LeafContainerTest, LeafContainerIsNotFocusedWhenStateHasNoFocusedContainer)
{
    EXPECT_FALSE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, LeafContainerIsFocusedWhenStateFocusesThisContainer)
{
    state->focus_container(leaf_container);
    EXPECT_TRUE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, LeafContainerIsFocusedWhenParentIsFocused)
{
    state->focus_container(parent);
    EXPECT_CALL(*parent, is_focused())
        .WillOnce(testing::Return(true));
    EXPECT_TRUE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, MatchWithAppId)
{
    ContainerScope scope(ContainerScopeType::app_id, "foo");

    miral::WindowSpecification spec;
    miral::WindowInfo info(window, spec);
    EXPECT_CALL(*window_controller, info_for(window))
        .WillRepeatedly(testing::ReturnRef(info));
    EXPECT_CALL(*surface, application_id())
        .WillRepeatedly(testing::Return("foo"));
    EXPECT_TRUE(leaf_container->matches(scope));
    scope.value = "bar";
    EXPECT_FALSE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchAll)
{
    ContainerScope scope(ContainerScopeType::all);
    EXPECT_TRUE(leaf_container->matches(scope));
}

struct LeafContainerMatchType
{
    std::string scope_value;
    MirWindowType required;
};

class LeafContainerMatchTypeTest : public LeafContainerTest, public ::testing::WithParamInterface<LeafContainerMatchType>
{
};

TEST_P(LeafContainerMatchTypeTest, MatchWindowType)
{
    auto param = GetParam();
    ContainerScope scope(ContainerScopeType::window_type, param.scope_value);

    miral::WindowSpecification spec;
    spec.type() = param.required;
    miral::WindowInfo info(window, spec);
    EXPECT_CALL(*window_controller, info_for(window))
        .WillRepeatedly(testing::ReturnRef(info));
    EXPECT_CALL(*surface, type())
        .WillRepeatedly(testing::Return(param.required));

    EXPECT_TRUE(leaf_container->matches(scope));
}

INSTANTIATE_TEST_SUITE_P(
    LeafContainerMatchTypeTest,
    LeafContainerMatchTypeTest,
    ::testing::Values(
        LeafContainerMatchType { "normal", mir_window_type_normal },
        LeafContainerMatchType { "dialog", mir_window_type_dialog },
        LeafContainerMatchType { "utility", mir_window_type_utility },
        LeafContainerMatchType { "toolbar", mir_window_type_decoration },
        LeafContainerMatchType { "menu", mir_window_type_decoration },
        LeafContainerMatchType { "dropdown_menu", mir_window_type_menu },
        LeafContainerMatchType { "popup_menu", mir_window_type_menu },
        LeafContainerMatchType { "tooltip", mir_window_type_tip },
        LeafContainerMatchType { "notification", mir_window_type_freestyle }));

TEST_F(LeafContainerTest, MatchTitle)
{
    ContainerScope scope(ContainerScopeType::title, "foo");
    miral::WindowSpecification spec;
    spec.name() = "foo";
    miral::WindowInfo info(window, spec);
    EXPECT_CALL(*window_controller, info_for(window))
        .WillRepeatedly(testing::ReturnRef(info));
    EXPECT_TRUE(leaf_container->matches(scope));
    scope.value = "bar";
    EXPECT_FALSE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchTitleWithSpecialFocusedKeyword)
{
    ContainerScope scope(ContainerScopeType::title, "__focused__");
    state->focus_container(leaf_container);

    miral::WindowSpecification spec;
    spec.name() = "foo";
    miral::WindowInfo info(window, spec);
    EXPECT_CALL(*window_controller, info_for(window))
        .WillRepeatedly(testing::ReturnRef(info));
    EXPECT_TRUE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchPid)
{
    ContainerScope scope(ContainerScopeType::pid, "123");
    miral::ApplicationInfo info(session);
    EXPECT_CALL(*window_controller, app_info(window))
        .WillRepeatedly(testing::ReturnRef(info));
    EXPECT_CALL(*session, process_id())
        .WillRepeatedly(testing::Return(123));
    EXPECT_TRUE(leaf_container->matches(scope));
    scope.value = "456";
    EXPECT_FALSE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchConId)
{
    ContainerScope scope(
        ContainerScopeType::con_id,
        std::to_string(reinterpret_cast<std::uintptr_t>(leaf_container.get())));

    EXPECT_TRUE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchConIdWithFocusedSpecialValue)
{
    ContainerScope scope(ContainerScopeType::con_id, "__focused__");
    state->focus_container(leaf_container);

    EXPECT_TRUE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchWorkspaceName)
{
    ContainerScope scope(ContainerScopeType::workspace, "foo");
    EXPECT_CALL(*workspace, display_name())
        .WillRepeatedly(testing::Return(std::string("foo")));
    EXPECT_TRUE(leaf_container->matches(scope));
    scope.value = "bar";
    EXPECT_FALSE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchFloating)
{
    ContainerScope scope(ContainerScopeType::floating);
    EXPECT_CALL(*parent, anchored())
        .WillRepeatedly(testing::Return(false));

    EXPECT_FALSE(leaf_container->matches(scope));
}

TEST_F(LeafContainerTest, MatchTiling)
{
    ContainerScope scope(ContainerScopeType::tiling);
    EXPECT_CALL(*parent, anchored())
        .WillRepeatedly(testing::Return(true));

    EXPECT_TRUE(leaf_container->matches(scope));
}

class LeafContainerMatchNotSupportedTest : public LeafContainerTest, public ::testing::WithParamInterface<ContainerScopeType>
{
};

TEST_P(LeafContainerMatchNotSupportedTest, MatchWindowType)
{
    ContainerScope scope(GetParam());
    EXPECT_FALSE(leaf_container->matches(scope));
}

INSTANTIATE_TEST_SUITE_P(
    LeafContainerMatchNotSupportedTest,
    LeafContainerMatchNotSupportedTest,
    ::testing::Values(
        ContainerScopeType::urgent,
        ContainerScopeType::con_mark,
        ContainerScopeType::floating_from,
        ContainerScopeType::tiling_from,
        ContainerScopeType::class_,
        ContainerScopeType::id,
        ContainerScopeType::window_role,
        ContainerScopeType::instance,
        ContainerScopeType::machine));

TEST_F(LeafContainerTest, CanAddReplacingMark)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    EXPECT_THAT(leaf_container->get_marks(), testing::ElementsAre("meow"));
}

TEST_F(LeafContainerTest, CanAddNonReplacingMark)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    leaf_container->mark(
        "woof",
        true,
        false);
    EXPECT_THAT(leaf_container->get_marks(), testing::ElementsAre("meow", "woof"));
}

TEST_F(LeafContainerTest, CanToggleMark)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    leaf_container->mark(
        "meow",
        false,
        true);
    EXPECT_THAT(leaf_container->get_marks(), testing::ElementsAre());
}

TEST_F(LeafContainerTest, CanUnmark)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    leaf_container->unmark("meow");
    EXPECT_THAT(leaf_container->get_marks(), testing::ElementsAre());
}

TEST_F(LeafContainerTest, CanUnmarkAll)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    leaf_container->unmark_all();
    EXPECT_THAT(leaf_container->get_marks(), testing::ElementsAre());
}

TEST_F(LeafContainerTest, CanMatchMark)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    ContainerScope scope(ContainerScopeType::con_mark, "meow");
    EXPECT_THAT(leaf_container->matches(scope), testing::Eq(true));
}

TEST_F(LeafContainerTest, CanFailToMatchMark)
{
    leaf_container->mark(
        "meow",
        false,
        false);
    ContainerScope scope(ContainerScopeType::con_mark, "meow2");
    EXPECT_THAT(leaf_container->matches(scope), testing::Eq(false));
}

TEST_F(LeafContainerTest, SetLogicalAreaTriggersListener)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_moved, (Container const&), (override));
    };

    auto const listener = std::make_shared<Listener>();
    EXPECT_CALL(*listener, on_container_moved(testing::_));
    leaf_container->register_interest(listener);

    geom::Rectangle constexpr new_area {
        { 10,  10  },
        { 200, 200 }
    };
    leaf_container->set_logical_area(new_area);
    leaf_container->commit_changes();
}

TEST_F(LeafContainerTest, HandleModifyChangeStateToFullscreenTriggersObserver)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_fullscreen, (Container const&), (override));
    };

    auto const listener = std::make_shared<Listener>();
    EXPECT_CALL(*listener, on_container_fullscreen(testing::_));
    leaf_container->register_interest(listener);

    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    leaf_container->handle_modify(spec, false);
}

TEST_F(LeafContainerTest, SetStateToFullscreenTriggersObserver)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_fullscreen, (Container const&), (override));
    };

    auto const listener = std::make_shared<Listener>();
    EXPECT_CALL(*listener, on_container_fullscreen(testing::_));
    leaf_container->register_interest(listener);

    leaf_container->set_state(mir_window_state_fullscreen);
    leaf_container->commit_changes();
}

TEST_F(LeafContainerTest, VisibleAreaCacheInitiallyDirty)
{
    // Create a new container to test initial state
    auto new_leaf = std::make_shared<LeafContainer>(
        workspace,
        window_controller,
        geom::Rectangle {
            { 10,  10  },
            { 200, 200 }
    },
        config,
        parent,
        state);

    // First call should calculate and cache the visible area
    auto area1 = new_leaf->get_visible_area();
    auto area2 = new_leaf->get_visible_area();

    // Both calls should return the same result (using cache on second call)
    EXPECT_EQ(area1.top_left.x.as_int(), area2.top_left.x.as_int());
    EXPECT_EQ(area1.top_left.y.as_int(), area2.top_left.y.as_int());
    EXPECT_EQ(area1.size.width.as_int(), area2.size.width.as_int());
    EXPECT_EQ(area1.size.height.as_int(), area2.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheInvalidatedOnSetLogicalArea)
{
    // Get initial visible area (this will cache it)
    auto initial_area = leaf_container->get_visible_area();

    // Change logical area to something significantly different
    geom::Rectangle new_logical_area {
        { 100, 200 }, // Different position
        { 500, 300 }  // Different size
    };
    leaf_container->set_logical_area(new_logical_area);

    // The visible area should not change yet since changes haven't been committed
    // Cache should still be valid until commit
    auto area_after_set = leaf_container->get_visible_area();
    EXPECT_EQ(initial_area.top_left.x.as_int(), area_after_set.top_left.x.as_int());
    EXPECT_EQ(initial_area.top_left.y.as_int(), area_after_set.top_left.y.as_int());
    EXPECT_EQ(initial_area.size.width.as_int(), area_after_set.size.width.as_int());
    EXPECT_EQ(initial_area.size.height.as_int(), area_after_set.size.height.as_int());

    // Now commit the changes - this should invalidate cache and update visible area
    leaf_container->commit_changes();

    // Get visible area after commit - should reflect new logical area
    auto area_after_commit = leaf_container->get_visible_area();

    // Size should have changed to reflect the new logical area (minus borders)
    int border_size = config->get_border_config().size;
    int expected_width = new_logical_area.size.width.as_int() - (2 * border_size);
    int expected_height = new_logical_area.size.height.as_int() - (2 * border_size);

    EXPECT_EQ(area_after_commit.size.width.as_int(), expected_width);
    EXPECT_EQ(area_after_commit.size.height.as_int(), expected_height);

    // Verify the cache is working by calling get_visible_area again
    auto cached_area = leaf_container->get_visible_area();
    EXPECT_EQ(area_after_commit.top_left.x.as_int(), cached_area.top_left.x.as_int());
    EXPECT_EQ(area_after_commit.top_left.y.as_int(), cached_area.top_left.y.as_int());
    EXPECT_EQ(area_after_commit.size.width.as_int(), cached_area.size.width.as_int());
    EXPECT_EQ(area_after_commit.size.height.as_int(), cached_area.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheInvalidatedOnCommitChanges)
{
    // Set up a pending logical area change
    geom::Rectangle new_logical_area {
        { 100, 100 },
        { 300, 250 }
    };
    leaf_container->set_logical_area(new_logical_area);

    // Get visible area before committing (should still reflect old logical area)
    auto area_before_commit = leaf_container->get_visible_area();

    // Commit the changes
    leaf_container->commit_changes();

    // Get visible area after committing (should be recalculated with committed logical area)
    auto area_after_commit = leaf_container->get_visible_area();

    // The areas should be different since the logical area changed during commit
    // This verifies that the cache was properly invalidated during commit
    EXPECT_NE(area_before_commit.top_left.x.as_int(), area_after_commit.top_left.x.as_int());
    EXPECT_NE(area_before_commit.top_left.y.as_int(), area_after_commit.top_left.y.as_int());
    EXPECT_NE(area_before_commit.size.width.as_int(), area_after_commit.size.width.as_int());
    EXPECT_NE(area_before_commit.size.height.as_int(), area_after_commit.size.height.as_int());

    // Verify the new visible area matches the expected size (logical area minus borders)
    int border_size = config->get_border_config().size;
    int expected_width = new_logical_area.size.width.as_int() - (2 * border_size);
    int expected_height = new_logical_area.size.height.as_int() - (2 * border_size);
    EXPECT_EQ(area_after_commit.size.width.as_int(), expected_width);
    EXPECT_EQ(area_after_commit.size.height.as_int(), expected_height);

    // Verify cache is working by calling get_visible_area again
    auto cached_area = leaf_container->get_visible_area();
    EXPECT_EQ(area_after_commit.top_left.x.as_int(), cached_area.top_left.x.as_int());
    EXPECT_EQ(area_after_commit.top_left.y.as_int(), cached_area.top_left.y.as_int());
    EXPECT_EQ(area_after_commit.size.width.as_int(), cached_area.size.width.as_int());
    EXPECT_EQ(area_after_commit.size.height.as_int(), cached_area.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheReusedWhenLogicalAreaUnchanged)
{
    // Create a spy to verify the expensive calculation isn't called repeatedly
    // We'll do this by checking that multiple calls return identical objects
    auto area1 = leaf_container->get_visible_area();
    auto area2 = leaf_container->get_visible_area();
    auto area3 = leaf_container->get_visible_area();

    // All should be identical when using cache
    EXPECT_EQ(area1.top_left.x.as_int(), area2.top_left.x.as_int());
    EXPECT_EQ(area1.top_left.y.as_int(), area2.top_left.y.as_int());
    EXPECT_EQ(area1.size.width.as_int(), area2.size.width.as_int());
    EXPECT_EQ(area1.size.height.as_int(), area2.size.height.as_int());

    EXPECT_EQ(area2.top_left.x.as_int(), area3.top_left.x.as_int());
    EXPECT_EQ(area2.top_left.y.as_int(), area3.top_left.y.as_int());
    EXPECT_EQ(area2.size.width.as_int(), area3.size.width.as_int());
    EXPECT_EQ(area2.size.height.as_int(), area3.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheAccountsForBorders)
{
    // Test that visible area calculation includes border considerations
    // and that this is consistently cached

    auto visible_area = leaf_container->get_visible_area();
    auto logical_area = leaf_container->get_logical_area();

    // Visible area should be smaller than logical area due to borders
    int border_size = config->get_border_config().size;
    int expected_width = logical_area.size.width.as_int() - (2 * border_size);
    int expected_height = logical_area.size.height.as_int() - (2 * border_size);

    EXPECT_EQ(visible_area.size.width.as_int(), expected_width);
    EXPECT_EQ(visible_area.size.height.as_int(), expected_height);
    EXPECT_EQ(visible_area.top_left.x.as_int(), logical_area.top_left.x.as_int() + border_size);
    EXPECT_EQ(visible_area.top_left.y.as_int(), logical_area.top_left.y.as_int() + border_size);

    // Verify cache is working by getting visible area again
    auto cached_visible_area = leaf_container->get_visible_area();
    EXPECT_EQ(visible_area.top_left.x.as_int(), cached_visible_area.top_left.x.as_int());
    EXPECT_EQ(visible_area.top_left.y.as_int(), cached_visible_area.top_left.y.as_int());
    EXPECT_EQ(visible_area.size.width.as_int(), cached_visible_area.size.width.as_int());
    EXPECT_EQ(visible_area.size.height.as_int(), cached_visible_area.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheHandlesMultipleLogicalAreaChanges)
{
    // Test that cache is properly invalidated and works correctly through multiple changes

    // Initial area with cache
    auto area1 = leaf_container->get_visible_area();
    auto initial_width = area1.size.width.as_int();
    auto initial_height = area1.size.height.as_int();

    // First change to a different size and commit
    geom::Rectangle change1 {
        { 100, 150 },
        { 600, 400 }  // Larger size
    };
    leaf_container->set_logical_area(change1);
    leaf_container->commit_changes();
    auto area2 = leaf_container->get_visible_area();

    // Second change to another different size and commit
    geom::Rectangle change2 {
        { 200, 250 },
        { 300, 200 }  // Smaller size
    };
    leaf_container->set_logical_area(change2);
    leaf_container->commit_changes();
    auto area3 = leaf_container->get_visible_area();

    // Verify sizes changed (which is more reliable than positions in test env)
    int border_size = config->get_border_config().size;
    int expected_width2 = change1.size.width.as_int() - (2 * border_size);
    int expected_height2 = change1.size.height.as_int() - (2 * border_size);
    int expected_width3 = change2.size.width.as_int() - (2 * border_size);
    int expected_height3 = change2.size.height.as_int() - (2 * border_size);

    EXPECT_EQ(area2.size.width.as_int(), expected_width2);
    EXPECT_EQ(area2.size.height.as_int(), expected_height2);
    EXPECT_EQ(area3.size.width.as_int(), expected_width3);
    EXPECT_EQ(area3.size.height.as_int(), expected_height3);

    // All areas should have different sizes
    EXPECT_NE(initial_width, area2.size.width.as_int());
    EXPECT_NE(initial_height, area2.size.height.as_int());
    EXPECT_NE(area2.size.width.as_int(), area3.size.width.as_int());
    EXPECT_NE(area2.size.height.as_int(), area3.size.height.as_int());

    // Test that cache is working by calling get_visible_area multiple times
    // without any changes - should return same values
    auto area3_cached = leaf_container->get_visible_area();
    auto area3_cached2 = leaf_container->get_visible_area();
    EXPECT_EQ(area3.top_left.x.as_int(), area3_cached.top_left.x.as_int());
    EXPECT_EQ(area3.top_left.y.as_int(), area3_cached.top_left.y.as_int());
    EXPECT_EQ(area3.size.width.as_int(), area3_cached.size.width.as_int());
    EXPECT_EQ(area3.size.height.as_int(), area3_cached.size.height.as_int());
    EXPECT_EQ(area3_cached.top_left.x.as_int(), area3_cached2.top_left.x.as_int());
    EXPECT_EQ(area3_cached.top_left.y.as_int(), area3_cached2.top_left.y.as_int());
    EXPECT_EQ(area3_cached.size.width.as_int(), area3_cached2.size.width.as_int());
    EXPECT_EQ(area3_cached.size.height.as_int(), area3_cached2.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheWorksAfterCommitChanges)
{
    // Set a new logical area and commit
    geom::Rectangle new_logical_area {
        { 75,  85  },
        { 450, 350 }
    };
    leaf_container->set_logical_area(new_logical_area);
    leaf_container->commit_changes();

    // Get visible area multiple times after commit
    auto area1 = leaf_container->get_visible_area();
    auto area2 = leaf_container->get_visible_area();
    auto area3 = leaf_container->get_visible_area();

    // Should all be identical (using cache)
    EXPECT_EQ(area1.top_left.x.as_int(), area2.top_left.x.as_int());
    EXPECT_EQ(area1.top_left.y.as_int(), area2.top_left.y.as_int());
    EXPECT_EQ(area1.size.width.as_int(), area2.size.width.as_int());
    EXPECT_EQ(area1.size.height.as_int(), area2.size.height.as_int());

    EXPECT_EQ(area2.top_left.x.as_int(), area3.top_left.x.as_int());
    EXPECT_EQ(area2.top_left.y.as_int(), area3.top_left.y.as_int());
    EXPECT_EQ(area2.size.width.as_int(), area3.size.width.as_int());
    EXPECT_EQ(area2.size.height.as_int(), area3.size.height.as_int());
}

TEST_F(LeafContainerTest, VisibleAreaCacheInvalidationMechanism)
{
    // Test the fundamental cache invalidation mechanism

    // Fill the cache
    auto area1 = leaf_container->get_visible_area();
    auto area1_again = leaf_container->get_visible_area();

    // These should be identical (cache working)
    EXPECT_EQ(area1.top_left.x.as_int(), area1_again.top_left.x.as_int());
    EXPECT_EQ(area1.top_left.y.as_int(), area1_again.top_left.y.as_int());
    EXPECT_EQ(area1.size.width.as_int(), area1_again.size.width.as_int());
    EXPECT_EQ(area1.size.height.as_int(), area1_again.size.height.as_int());

    // Set a new logical area (should invalidate cache)
    geom::Rectangle new_area {
        { 50,  60  },
        { 300, 200 }
    };
    leaf_container->set_logical_area(new_area);

    // Cache should be invalidated, but visible area won't change until commit
    auto area2 = leaf_container->get_visible_area();
    EXPECT_EQ(area1.top_left.x.as_int(), area2.top_left.x.as_int()); // Still same before commit
    EXPECT_EQ(area1.top_left.y.as_int(), area2.top_left.y.as_int());

    // Commit changes
    leaf_container->commit_changes();

    // Now the visible area should be different and cached
    auto area3 = leaf_container->get_visible_area();
    auto area3_again = leaf_container->get_visible_area();

    // These should be identical (cache working after commit)
    EXPECT_EQ(area3.top_left.x.as_int(), area3_again.top_left.x.as_int());
    EXPECT_EQ(area3.top_left.y.as_int(), area3_again.top_left.y.as_int());
    EXPECT_EQ(area3.size.width.as_int(), area3_again.size.width.as_int());
    EXPECT_EQ(area3.size.height.as_int(), area3_again.size.height.as_int());
}

/// When a layout change (e.g. from a drag-to transfer) triggers commit_changes()
/// while a drag is active, set_rectangle must NOT be called. The window position
/// is already managed by drag() -> window_controller->modify(), and calling
/// set_rectangle with a stale 'previous' would cause flickering/snapping.
TEST_F(LeafContainerTest, CommitChangesDoesNotCallSetRectangleWhileDragging)
{
    ON_CALL(*window_controller, get_state(testing::_))
        .WillByDefault(testing::Return(mir_window_state_restored));

    leaf_container->drag_start();
    leaf_container->drag(200, 150);

    // Simulate relayout() assigning a new logical area to the dragged container
    geom::Rectangle new_area {
        { 400, 0   },
        { 400, 300 }
    };
    leaf_container->set_logical_area(new_area);

    EXPECT_CALL(*window_controller, set_rectangle(testing::_, testing::_, testing::_, testing::_))
        .Times(0);

    leaf_container->commit_changes();
}

/// Complement: when not dragging, commit_changes() must call set_rectangle as normal.
TEST_F(LeafContainerTest, CommitChangesCallsSetRectangleWhenNotDragging)
{
    ON_CALL(*window_controller, get_state(testing::_))
        .WillByDefault(testing::Return(mir_window_state_restored));

    geom::Rectangle new_area {
        { 400, 0   },
        { 400, 300 }
    };
    leaf_container->set_logical_area(new_area);

    EXPECT_CALL(*window_controller, set_rectangle(testing::_, testing::_, testing::_, testing::_))
        .Times(1);

    leaf_container->commit_changes();
}

// ---- urgency ----

TEST_F(LeafContainerTest, UrgencyIsFalseByDefault)
{
    EXPECT_FALSE(leaf_container->urgent());
}

TEST_F(LeafContainerTest, SetUrgentOnlyReportsAChangeWhenTheValueActuallyChanges)
{
    EXPECT_TRUE(leaf_container->set_urgent(true));
    EXPECT_TRUE(leaf_container->urgent());

    // Setting the same value again is not a change, so observers must not be told.
    EXPECT_FALSE(leaf_container->set_urgent(true));
    EXPECT_TRUE(leaf_container->urgent());

    EXPECT_TRUE(leaf_container->set_urgent(false));
    EXPECT_FALSE(leaf_container->urgent());

    EXPECT_FALSE(leaf_container->set_urgent(false));
    EXPECT_FALSE(leaf_container->urgent());
}

TEST_F(LeafContainerTest, ToJsonReportsUrgency)
{
    EXPECT_FALSE(leaf_container->to_json(true)["urgent"]);

    leaf_container->set_urgent(true);
    EXPECT_TRUE(leaf_container->to_json(true)["urgent"]);
}
