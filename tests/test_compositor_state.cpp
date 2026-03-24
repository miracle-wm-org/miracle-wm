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
#include "leaf_container.h"
#include "mock_output.h"
#include "mock_parent_container.h"
#include "mock_session.h"
#include "mock_shell_application_spawner.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "parent_container.h"
#include "shell_application_manager.h"
#include "stub_configuration.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace miracle;
using namespace testing;
namespace geom = mir::geometry;

namespace
{
auto parent_area = geom::Rectangle {
    { 0,   0   },
    { 800, 600 }
};
}

// ──────────────────────────────────────────────────────────────────────────────
// CompositorState::next_container_id tests
// ──────────────────────────────────────────────────────────────────────────────

TEST(CompositorStateTest, FirstIdIsOne)
{
    CompositorState state;
    EXPECT_EQ(1u, state.next_container_id());
}

TEST(CompositorStateTest, NextContainerIdIncrementsMonotonically)
{
    CompositorState state;
    auto const id1 = state.next_container_id();
    auto const id2 = state.next_container_id();
    auto const id3 = state.next_container_id();
    EXPECT_EQ(id1 + 1, id2);
    EXPECT_EQ(id2 + 1, id3);
}

TEST(CompositorStateTest, IndependentStatesHaveIndependentCounters)
{
    CompositorState state1;
    CompositorState state2;
    EXPECT_EQ(state1.next_container_id(), state2.next_container_id());
}

// ──────────────────────────────────────────────────────────────────────────────
// Container ID assignment tests (via concrete subclasses)
// ──────────────────────────────────────────────────────────────────────────────

class ContainerIdTest : public Test
{
public:
    ContainerIdTest() :
        workspace(std::make_shared<NiceMock<test::MockWorkspace>>()),
        parent(std::make_shared<NiceMock<test::MockParentContainer>>()),
        session(std::make_shared<NiceMock<test::MockSession>>()),
        surface(std::make_shared<NiceMock<test::MockSurface>>()),
        app(session),
        window(app, surface)
    {
        workspaces.push_back(workspace);

        ON_CALL(*workspace, get_output())
            .WillByDefault(Return(output));
        ON_CALL(*output, get_area())
            .WillByDefault(ReturnRef(parent_area));
        ON_CALL(*output, get_workspaces())
            .WillByDefault(ReturnRef(workspaces));
    }

    std::shared_ptr<CompositorState> make_state() { return std::make_shared<CompositorState>(); }

    std::shared_ptr<LeafContainer> make_leaf(std::shared_ptr<CompositorState> const& state)
    {
        auto leaf = std::make_shared<LeafContainer>(
            workspace,
            window_controller,
            geom::Rectangle {
                { 0,   0   },
                { 400, 300 }
        },
            config,
            parent,
            state);
        state->add(leaf);
        leaf->associate_to_window(window);
        return leaf;
    }

protected:
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
    std::shared_ptr<Config> config = std::make_shared<test::StubConfiguration>();
    std::shared_ptr<test::MockWorkspace> workspace;
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    std::shared_ptr<test::MockOutput> output = std::make_shared<NiceMock<test::MockOutput>>();
    std::shared_ptr<test::MockParentContainer> parent;
    std::shared_ptr<test::MockSession> session;
    std::shared_ptr<test::MockSurface> surface;
    miral::Application app;
    miral::Window window;
};

TEST_F(ContainerIdTest, LeafContainerGetsNonZeroId)
{
    auto state = make_state();
    auto leaf = make_leaf(state);
    EXPECT_GT(leaf->id(), 0u);
}

TEST_F(ContainerIdTest, TwoLeafContainersGetDifferentIds)
{
    auto state = make_state();
    auto leaf1 = make_leaf(state);
    auto leaf2 = make_leaf(state);
    EXPECT_NE(leaf1->id(), leaf2->id());
}

TEST_F(ContainerIdTest, ContainerIdsAreStrictlyIncreasing)
{
    auto state = make_state();
    auto leaf1 = make_leaf(state);
    auto leaf2 = make_leaf(state);
    EXPECT_LT(leaf1->id(), leaf2->id());
}

TEST_F(ContainerIdTest, FirstLeafContainerGetsFirstIdFromState)
{
    auto state = make_state();
    // Capture what the next id will be before creating the container.
    // Since next_container_id() increments on each call, we create a fresh
    // state and verify the first container gets id 1.
    auto fresh_state = make_state();
    auto leaf = make_leaf(fresh_state);
    EXPECT_EQ(1u, leaf->id());
}

TEST_F(ContainerIdTest, ParentContainerGetsIdFromSamePool)
{
    auto state = make_state();
    // LeafContainer takes id 1 from state.
    auto leaf = make_leaf(state);

    auto shell_app_manager = std::make_shared<ShellApplicationManager>(
        std::make_unique<NiceMock<test::MockShellApplicationSpawner>>());

    // ParentContainer takes the next id (2) from the same state.
    auto pc = std::make_shared<ParentContainer>(
        shell_app_manager,
        state,
        window_controller,
        config,
        geom::Rectangle {
            { 0,   0   },
            { 800, 600 }
    },
        workspace,
        nullptr,
        true);

    EXPECT_GT(pc->id(), 0u);
    EXPECT_NE(leaf->id(), pc->id());
    EXPECT_LT(leaf->id(), pc->id());
}
