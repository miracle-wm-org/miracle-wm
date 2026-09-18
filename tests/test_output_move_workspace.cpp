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
#include "mock_output_factory.h"
#include "mock_shell_application_spawner.h"
#include "mock_window_controller.h"
#include "output.h"
#include "output_manager.h"
#include "shell_application_manager.h"
#include "stub_configuration.h"
#include "workspace_manager.h"
#include "workspace_observer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <mir/geometry/rectangle.h>

using namespace miracle;
using namespace testing;

namespace
{
/// Hands out the provided outputs in order, one per [create] call.
std::unique_ptr<NiceMock<test::MockOutputFactory>> create_output_factory(
    std::vector<std::shared_ptr<AbstractOutput>> const& outputs)
{
    auto output_factory = std::make_unique<NiceMock<test::MockOutputFactory>>();
    auto next = std::make_shared<size_t>(0);
    ON_CALL(*output_factory, create)
        .WillByDefault(Invoke([&outputs, next](std::string, int, geom::Rectangle) -> std::shared_ptr<AbstractOutput>
    {
        if (*next >= outputs.size())
            return nullptr;
        return outputs[(*next)++];
    }));
    return output_factory;
}

class FocusObserver : public NullWorkspaceObserver
{
public:
    void on_workspace_focused(std::optional<uint32_t> old, uint32_t next) override
    {
        focused.emplace_back(old, next);
    }

    std::vector<std::pair<std::optional<uint32_t>, uint32_t>> focused;
};
}

/// Exercises [Output::move_workspace_to] with two real outputs so that the
/// bookkeeping on both ends of the move (active workspace, output focus,
/// focus events) is covered.
class OutputMoveWorkspaceTest : public Test
{
public:
    void SetUp() override
    {
        window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
        animator = std::make_shared<Animator>();
        state = std::make_shared<CompositorState>();
        config = std::make_shared<test::StubConfiguration>();
        shell_application_manager = std::make_shared<ShellApplicationManager>(
            std::make_unique<NiceMock<test::MockShellApplicationSpawner>>());
        registrar = std::make_shared<WorkspaceObserverRegistrar>();

        output_a = create_output("A", 1, geom::Rectangle {
                                             { 0,    0    },
                                             { 1920, 1080 }
        });
        output_b = create_output("B", 2, geom::Rectangle {
                                             { 1920, 0    },
                                             { 1920, 1080 }
        });
        outputs_to_create = { output_a, output_b };

        output_manager = std::make_shared<OutputManager>(create_output_factory(outputs_to_create));
        workspace_manager = std::make_shared<WorkspaceManager>(registrar, config, output_manager);

        observer = std::make_shared<FocusObserver>();
        registrar->register_interest(observer);

        // A = [ws1 (num 1), active], B = [ws2 (num 2), active], focused output = A
        output_manager->create("A", 1, output_a->get_area(), *workspace_manager);
        output_manager->create("B", 2, output_b->get_area(), *workspace_manager);
        ASSERT_EQ(output_manager->focused(), output_a);
        ASSERT_EQ(output_a->get_workspaces().size(), 1);
        ASSERT_EQ(output_b->get_workspaces().size(), 1);
        ws1 = output_a->active();
        ws2 = output_b->active();
        ASSERT_EQ(ws1->num(), 1);
        ASSERT_EQ(ws2->num(), 2);
        observer->focused.clear();
    }

    std::shared_ptr<Output> create_output(std::string const& name, int id, geom::Rectangle const& area)
    {
        return std::make_shared<Output>(
            shell_application_manager,
            name,
            id,
            area,
            OutputConfigDetails {},
            state,
            config,
            window_controller,
            animator,
            make_null_plugin_manager());
    }

    /// Adds a workspace to [output] without activating it. Ids are kept well
    /// above anything that [WorkspaceManager] will hand out.
    std::shared_ptr<AbstractWorkspace> add_workspace(std::shared_ptr<Output> const& output, uint32_t id, int num)
    {
        output->advise_new_workspace(WorkspaceCreationData {
            .id = id,
            .num = num,
            .name = std::nullopt,
            .registrar = registrar });
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->id() == id)
                return workspace;
        }
        return nullptr;
    }

    std::shared_ptr<NiceMock<test::MockWindowController>> window_controller;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<test::StubConfiguration> config;
    std::shared_ptr<ShellApplicationManager> shell_application_manager;
    std::shared_ptr<WorkspaceObserverRegistrar> registrar;
    std::shared_ptr<Output> output_a;
    std::shared_ptr<Output> output_b;
    std::vector<std::shared_ptr<AbstractOutput>> outputs_to_create;
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<FocusObserver> observer;
    std::shared_ptr<AbstractWorkspace> ws1;
    std::shared_ptr<AbstractWorkspace> ws2;
};

TEST_F(OutputMoveWorkspaceTest, MovingTheActiveWorkspaceSelectsTheNextOneOnTheOldOutput)
{
    add_workspace(output_a, 100, 5);
    ASSERT_EQ(output_a->get_workspaces().size(), 2);
    ASSERT_EQ(output_a->active(), ws1);

    workspace_manager->move_workspace_to_output(ws1->id(), output_b.get());

    // The old output falls back to its remaining workspace...
    ASSERT_EQ(output_a->get_workspaces().size(), 1);
    ASSERT_NE(output_a->active(), nullptr);
    EXPECT_EQ(output_a->active()->id(), 100);

    // ...while the moved workspace is active and focused on the new output.
    // B's empty ws2 is deleted when the moved workspace takes over.
    EXPECT_EQ(output_b->active(), ws1);
    EXPECT_EQ(ws1->get_output(), output_b);
    EXPECT_EQ(output_b->get_workspaces().size(), 1);
    EXPECT_EQ(output_manager->focused(), output_b);

    ASSERT_FALSE(observer->focused.empty());
    EXPECT_EQ(observer->focused.back().first, std::optional<uint32_t>(100));
    EXPECT_EQ(observer->focused.back().second, ws1->id());
}

TEST_F(OutputMoveWorkspaceTest, MovingTheOnlyWorkspaceGivesTheOldOutputANewOne)
{
    ASSERT_EQ(output_a->get_workspaces().size(), 1);

    workspace_manager->move_workspace_to_output(ws1->id(), output_b.get());

    // The old output receives the first free workspace number (1 and 2 are taken).
    ASSERT_EQ(output_a->get_workspaces().size(), 1);
    auto const replacement = output_a->active();
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement, output_a->get_workspaces()[0]);
    EXPECT_EQ(replacement->num(), 3);
    EXPECT_NE(replacement, ws1);

    EXPECT_EQ(output_b->active(), ws1);
    EXPECT_EQ(ws1->get_output(), output_b);
    EXPECT_EQ(output_b->get_workspaces().size(), 1);
    EXPECT_EQ(output_manager->focused(), output_b);

    // The replacement is created while the old output has no active workspace,
    // and the moved workspace is focused last so that focus lands on it.
    ASSERT_EQ(observer->focused.size(), 2);
    EXPECT_EQ(observer->focused[0].first, std::nullopt);
    EXPECT_EQ(observer->focused[0].second, replacement->id());
    EXPECT_EQ(observer->focused[1].first, std::optional<uint32_t>(replacement->id()));
    EXPECT_EQ(observer->focused[1].second, ws1->id());
}

TEST_F(OutputMoveWorkspaceTest, MovingANonActiveWorkspaceLeavesTheOldOutputAlone)
{
    auto const ws100 = add_workspace(output_a, 100, 5);
    ASSERT_EQ(output_a->get_workspaces().size(), 2);
    ASSERT_EQ(output_a->active(), ws1);

    workspace_manager->move_workspace_to_output(ws100->id(), output_b.get());

    EXPECT_EQ(output_a->active(), ws1);
    EXPECT_EQ(output_a->get_workspaces().size(), 1);

    EXPECT_EQ(output_b->active(), ws100);
    EXPECT_EQ(ws100->get_output(), output_b);
    EXPECT_EQ(output_b->get_workspaces().size(), 1);
    EXPECT_EQ(output_manager->focused(), output_b);

    ASSERT_FALSE(observer->focused.empty());
    EXPECT_EQ(observer->focused.back().first, std::optional<uint32_t>(ws1->id()));
    EXPECT_EQ(observer->focused.back().second, ws100->id());
}
