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
std::unique_ptr<NiceMock<test::MockOutputFactory>> create_output_factory(std::shared_ptr<OutputInterface> const& output)
{
    auto output_factory = std::make_unique<NiceMock<test::MockOutputFactory>>();
    ON_CALL(*output_factory, create)
        .WillByDefault(Return(output));
    return output_factory;
}
}

class WorkspaceManagerTest : public Test
{
public:
    WorkspaceManagerTest() :
        workspace_registry(std::make_shared<WorkspaceObserverRegistrar>()),
        config(std::make_shared<NiceMock<test::MockConfig>>()),
        output_manager(std::make_shared<OutputManager>(create_output_factory(output))),
        workspace_manager(workspace_registry, config, output_manager)
    {
        ON_CALL(*output, active).WillByDefault(Invoke([this]
        { return active_workspace; }));

        ON_CALL(*output, get_workspaces)
            .WillByDefault(ReturnRef(workspaces));

        ON_CALL(*output, advise_new_workspace)
            .WillByDefault(Invoke([this](WorkspaceCreationData const& data)
        {
            add_new_workspace(data);
        }));

        ON_CALL(*output, advise_workspace_active)
            .WillByDefault(Invoke([this](WorkspaceManager& manager, uint32_t id) -> bool
        {
            auto iter = std::ranges::find_if(workspaces, [id](const auto& workspace)
            {
                return workspace->id() == id;
            });

            if (iter != workspaces.end())
            {
                active_workspace = *iter;
                return true;
            }

            return false;
        }));
    }

    void add_new_workspace(WorkspaceCreationData const& data)
    {
        auto created_workspace = std::make_shared<testing::NiceMock<test::MockWorkspace>>();
        ON_CALL(*created_workspace, id())
            .WillByDefault(::testing::Return(data.id));
        ON_CALL(*created_workspace, num())
            .WillByDefault(::testing::Return(data.num));
        ON_CALL(*created_workspace, get_output)
            .WillByDefault(::testing::Return(output));
        workspaces.push_back(created_workspace);
    }

    void create_output()
    {
        output_manager->create("Output1", 1, {
                                                 { 0,    0    },
                                                 { 1920, 1080 }
        },
            workspace_manager);
    }

    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    std::shared_ptr<WorkspaceInterface> active_workspace;
    std::shared_ptr<NiceMock<test::MockOutput>> output = std::make_shared<NiceMock<test::MockOutput>>();
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
    EXPECT_TRUE(first_workspace->id() == 0);

    EXPECT_TRUE(active_workspace == first_workspace);

    // Request workspace 2 should create a new workspace and make it active
    workspace_manager.request_workspace(output.get(), 2);

    EXPECT_TRUE(workspaces.size() == 2);
    const auto& second_workspace = workspaces.back();
    EXPECT_TRUE(second_workspace->get_output() == output);
    EXPECT_TRUE(second_workspace->num() == 2);
    EXPECT_TRUE(second_workspace->id() == 1);

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
