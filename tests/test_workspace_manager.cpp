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

#include "mock_configuration.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_workspace.h"
#include "output_manager.h"
#include "workspace_manager.h"
#include "workspace_observer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
}

class WorkspaceManagerTest : public Test
{
public:
    WorkspaceManagerTest() :
        workspace_registry(std::make_shared<WorkspaceObserverRegistrar>()),
        config(std::make_shared<NiceMock<test::MockConfig>>()),
        output_manager(std::make_shared<OutputManager>(create_output_factory(outputs_to_create))),
        workspace_manager(workspace_registry, config, output_manager)
    {
        setup_output(output, 1, workspaces, active_workspace);
        setup_output(second_output, 2, second_workspaces, second_active_workspace);
    }

    void setup_output(
        std::shared_ptr<NiceMock<test::MockOutput>> const& target,
        int id,
        std::vector<std::shared_ptr<AbstractWorkspace>>& target_workspaces,
        std::shared_ptr<AbstractWorkspace>& target_active)
    {
        ON_CALL(*target, id()).WillByDefault(Return(id));

        ON_CALL(*target, active).WillByDefault(Invoke([&target_active]
        { return target_active; }));

        ON_CALL(*target, get_workspaces)
            .WillByDefault(Invoke([&target_workspaces]()
        { return target_workspaces; }));

        ON_CALL(*target, advise_new_workspace)
            .WillByDefault(Invoke([this, target](WorkspaceCreationData const& data)
        {
            add_new_workspace(data, target);
        }));

        ON_CALL(*target, advise_workspace_active)
            .WillByDefault(Invoke([&target_workspaces, &target_active](WorkspaceManager& manager, uint32_t id) -> bool
        {
            auto iter = std::ranges::find_if(target_workspaces, [id](const auto& workspace)
            {
                return workspace->id() == id;
            });

            if (iter != target_workspaces.end())
            {
                target_active = *iter;
                return true;
            }

            return false;
        }));
    }

    void add_new_workspace(
        WorkspaceCreationData const& data,
        std::shared_ptr<NiceMock<test::MockOutput>> const& target)
    {
        auto created_workspace = std::make_shared<testing::NiceMock<test::MockWorkspace>>();
        ON_CALL(*created_workspace, id())
            .WillByDefault(::testing::Return(data.id));
        ON_CALL(*created_workspace, num())
            .WillByDefault(::testing::Return(data.num));
        ON_CALL(*created_workspace, get_output)
            .WillByDefault(::testing::Return(target));
        if (target == output)
            workspaces.push_back(created_workspace);
        else
            second_workspaces.push_back(created_workspace);
    }

    void create_output()
    {
        output_manager->create("Output1", 1, {
                                                 { 0,    0    },
                                                 { 1920, 1080 }
        },
            workspace_manager);
    }

    void create_second_output()
    {
        output_manager->create("Output2", 2, {
                                                 { 1920, 0    },
                                                 { 1920, 1080 }
        },
            workspace_manager);
    }

    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    std::shared_ptr<AbstractWorkspace> active_workspace;
    std::vector<std::shared_ptr<AbstractWorkspace>> second_workspaces;
    std::shared_ptr<AbstractWorkspace> second_active_workspace;
    std::shared_ptr<NiceMock<test::MockOutput>> output = std::make_shared<NiceMock<test::MockOutput>>();
    std::shared_ptr<NiceMock<test::MockOutput>> second_output = std::make_shared<NiceMock<test::MockOutput>>();
    std::vector<std::shared_ptr<AbstractOutput>> outputs_to_create { output, second_output };
    std::shared_ptr<WorkspaceObserverRegistrar> workspace_registry;
    std::shared_ptr<test::MockConfig> config;
    std::shared_ptr<OutputManager> output_manager;
    WorkspaceManager workspace_manager;
};

TEST_F(WorkspaceManagerTest, RequestNewWorkspace)
{
    // Creating a single output should result in one workspace being created
    create_output();

    EXPECT_TRUE(workspaces.size() == 1);

    const auto& first_workspace = workspaces.front();
    EXPECT_TRUE(first_workspace->get_output() == output);
    EXPECT_TRUE(first_workspace->num() == 1);
    EXPECT_TRUE(first_workspace->id() == 1);

    EXPECT_TRUE(active_workspace == first_workspace);

    // Request workspace 2 should create a new workspace and make it active
    workspace_manager.request_workspace(output.get(), 2);

    EXPECT_TRUE(workspaces.size() == 2);
    const auto& second_workspace = workspaces.back();
    EXPECT_TRUE(second_workspace->get_output() == output);
    EXPECT_TRUE(second_workspace->num() == 2);
    EXPECT_TRUE(second_workspace->id() == 2);

    EXPECT_TRUE(active_workspace == second_workspace);
}

TEST_F(WorkspaceManagerTest, RequestWorkspaceBackAndForthEnabled)
{
    create_output();

    workspace_manager.request_workspace(output.get(), 2);

    // ensure second workspace is created and active
    EXPECT_TRUE(workspaces.size() == 2);
    EXPECT_TRUE(active_workspace->num() == 2);

    // no new workspace should be when we request existing num 1
    workspace_manager.request_workspace(output.get(), 1);
    EXPECT_TRUE(workspaces.size() == 2);
    EXPECT_TRUE(active_workspace->num() == 1);

    // request currently active workspace - should switch to previously active one
    EXPECT_CALL(*config, get_workspace_back_and_forth).WillOnce(Return(true));
    workspace_manager.request_workspace(output.get(), 1);
    EXPECT_TRUE(active_workspace->num() == 2);
}

TEST_F(WorkspaceManagerTest, RequestWorkspaceByNumCreatesAndReturns)
{
    create_output();

    auto* ws = workspace_manager.request_workspace(output.get(), 5, std::nullopt, false);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->num(), 5);
    EXPECT_EQ(workspaces.size(), 2u); // 1 default + 1 new
}

TEST_F(WorkspaceManagerTest, RequestWorkspaceByNumReturnsExisting)
{
    create_output();

    // Workspace 1 was created by create_output
    auto* ws = workspace_manager.request_workspace(output.get(), 1, std::nullopt, false);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->num(), 1);
    EXPECT_EQ(workspaces.size(), 1u); // no new workspace created
}

TEST_F(WorkspaceManagerTest, RequestWorkspaceByNumWithFocusChangesActive)
{
    create_output();

    workspace_manager.request_workspace(output.get(), 2);
    EXPECT_EQ(active_workspace->num(), 2);

    // Request workspace 1 with focus=true should switch active
    auto* ws = workspace_manager.request_workspace(output.get(), 1, std::nullopt, true);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(active_workspace->num(), 1);
}

TEST_F(WorkspaceManagerTest, RequestWorkspaceByNumWithoutFocusDoesNotChangeActive)
{
    create_output();

    // Active is workspace 1
    EXPECT_EQ(active_workspace->num(), 1);

    // Create workspace 2 without focus
    auto* ws = workspace_manager.request_workspace(output.get(), 2, std::nullopt, false);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->num(), 2);

    // Active should still be workspace 1
    EXPECT_EQ(active_workspace->num(), 1);
}

TEST_F(WorkspaceManagerTest, RequestWorkspaceBackAndForthDisabled)
{
    create_output();

    workspace_manager.request_workspace(output.get(), 2);

    // ensure second workspace is created and active
    EXPECT_TRUE(workspaces.size() == 2);
    EXPECT_TRUE(active_workspace->num() == 2);

    // no new workspace should be created when we request existing num 1
    workspace_manager.request_workspace(output.get(), 1);
    EXPECT_TRUE(workspaces.size() == 2);
    EXPECT_TRUE(active_workspace->num() == 1);

    // active workspace shoudn't change with workspace_back_and_forth == false
    EXPECT_CALL(*config, get_workspace_back_and_forth).WillOnce(Return(false));
    workspace_manager.request_workspace(output.get(), 1);
    EXPECT_TRUE(active_workspace->num() == 1);
}

TEST_F(WorkspaceManagerTest, RequestFocusSelectsAWindowOnTheAlreadyActiveWorkspace)
{
    create_output();
    ASSERT_EQ(workspaces.size(), 1);

    // The workspace is already the active one on its output, so it is never
    // re-shown. It must still be given focus.
    auto const active = std::dynamic_pointer_cast<NiceMock<test::MockWorkspace>>(active_workspace);
    ASSERT_NE(active, nullptr);
    EXPECT_CALL(*active, select_window()).Times(1);

    workspace_manager.request_focus(active->id());
}

TEST_F(WorkspaceManagerTest, RequestFocusOnAnotherOutputMovesOutputFocusAndSelectsAWindow)
{
    create_output();
    create_second_output();

    // Creating an output must not steal focus from the already focused one.
    ASSERT_EQ(output_manager->focused(), output);
    ASSERT_EQ(second_workspaces.size(), 1);

    auto const target = std::dynamic_pointer_cast<NiceMock<test::MockWorkspace>>(second_workspaces[0]);
    ASSERT_NE(target, nullptr);
    EXPECT_CALL(*target, select_window()).Times(1);

    workspace_manager.request_focus(target->id());

    EXPECT_EQ(output_manager->focused(), second_output);
}
