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

#include "compositor_state.h"
#include "config.h"
#include "container_group_container.h"
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
        parent(std::make_shared<testing::NiceMock<test::MockParentContainer>>(
            state,
            window_controller,
            config,
            parent_area,
            workspace,
            nullptr,
            true)),
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
            .WillByDefault(testing::ReturnRef(workspaces));

        state->add(leaf_container);
        leaf_container->associate_to_window(window);
    }

protected:
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<testing::NiceMock<test::MockWindowController>>();
    std::shared_ptr<Config> config = std::make_shared<test::StubConfiguration>();
    std::shared_ptr<test::MockWorkspace> workspace;
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    std::shared_ptr<test::MockOutput> output = std::make_shared<testing::NiceMock<test::MockOutput>>();
    std::shared_ptr<test::MockParentContainer> parent;
    std::shared_ptr<LeafContainer> leaf_container;
    std::shared_ptr<test::MockSession> session;
    std::shared_ptr<test::MockSurface> surface;
    miral::Application app;
    miral::Window window;
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

    leaf_container->set_workspace(new_workspace);
    ASSERT_EQ(leaf_container->get_workspace(), new_workspace);
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

TEST_F(LeafContainerTest, IfParentIsUnanchoredThenParentCanBeResizedLeft)
{
    ON_CALL(*parent, anchored())
        .WillByDefault(testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(testing::Return(1));
    ON_CALL(*parent, get_area())
        .WillByDefault(testing::Return(parent_area));

    geom::Rectangle result(
        geom::Point { 0, 0 },
        geom::Size { parent_area.size.width.as_int() - 20, parent_area.size.height.as_int() });
    EXPECT_CALL(*parent, set_logical_area(result, true));
    EXPECT_CALL(*parent, commit_changes());
    leaf_container->resize(Direction::left, 20);
}

TEST_F(LeafContainerTest, IfParentIsUnanchoredThenParentCanBeResizedRight)
{
    ON_CALL(*parent, anchored())
        .WillByDefault(testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(testing::Return(1));
    ON_CALL(*parent, get_area())
        .WillByDefault(testing::Return(parent_area));

    geom::Rectangle result(
        geom::Point { 0, 0 },
        geom::Size { parent_area.size.width.as_int() + 20, parent_area.size.height.as_int() });
    EXPECT_CALL(*parent, set_logical_area(result, true));
    EXPECT_CALL(*parent, commit_changes());
    leaf_container->resize(Direction::right, 20);
}

TEST_F(LeafContainerTest, IfParentIsUnanchoredThenParentCanBeResizedUp)
{
    ON_CALL(*parent, anchored())
        .WillByDefault(testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(testing::Return(1));
    ON_CALL(*parent, get_area())
        .WillByDefault(testing::Return(parent_area));

    geom::Rectangle result(
        geom::Point { 0, 0 },
        geom::Size { parent_area.size.width.as_int(), parent_area.size.height.as_int() - 20 });
    EXPECT_CALL(*parent, set_logical_area(result, true));
    EXPECT_CALL(*parent, commit_changes());
    leaf_container->resize(Direction::up, 20);
}

TEST_F(LeafContainerTest, IfParentIsUnanchoredThenParentCanBeResizedDown)
{
    ON_CALL(*parent, anchored())
        .WillByDefault(testing::Return(false));
    ON_CALL(*parent, num_nodes())
        .WillByDefault(testing::Return(1));
    ON_CALL(*parent, get_area())
        .WillByDefault(testing::Return(parent_area));

    geom::Rectangle result(
        geom::Point { 0, 0 },
        geom::Size { parent_area.size.width.as_int(), parent_area.size.height.as_int() + 20 });
    EXPECT_CALL(*parent, set_logical_area(result, true));
    EXPECT_CALL(*parent, commit_changes());
    leaf_container->resize(Direction::down, 20);
}

TEST_F(LeafContainerTest, IfModifyingWindowToFullScreenThenNoclipIsCalled)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    EXPECT_CALL(*window_controller, noclip(testing::_));
    leaf_container->handle_modify(spec);
}

TEST_F(LeafContainerTest, IfModifyingWindowToRestoredThenClipIsCalled)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_restored;
    EXPECT_CALL(*window_controller, clip(testing::_, testing::_));
    leaf_container->handle_modify(spec);
}

namespace
{
bool has_restored_state(miral::WindowSpecification const& spec)
{
    return spec.state().is_set() && spec.state().value() == mir_window_state_restored;
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
    leaf_container->handle_modify(spec);
}

INSTANTIATE_TEST_SUITE_P(
    LeafContainerMaximizedTest,
    LeafContainerMaximizedTest,
    ::testing::Values(mir_window_state_maximized, mir_window_state_vertmaximized, mir_window_state_horizmaximized, mir_window_state_minimized, mir_window_state_hidden));

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
    state->focus_container(parent, true);
    EXPECT_CALL(*parent, is_focused())
        .WillOnce(testing::Return(true));
    EXPECT_TRUE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, LeafContainerIsFocusedWhenGroupIsFocused)
{
    auto container_group_container = std::make_shared<ContainerGroupContainer>(
        state);
    state->focus_container(container_group_container, true);
    container_group_container->add(leaf_container);
    EXPECT_CALL(*parent, is_focused())
        .WillOnce(testing::Return(false));
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

    EXPECT_TRUE(leaf_container->matches(scope));
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

TEST_F(LeafContainerTest, CanSetAlpha)
{
    EXPECT_CALL(*surface, set_transformation(leaf_container->get_transform()));
    leaf_container->set_alpha(0.5f);

    auto data = state->render_data_manager()->get();
    EXPECT_EQ(data.size(), 1);
    EXPECT_EQ(data[0].alpha, 0.5f);
}

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
    leaf_container->handle_modify(spec);
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
