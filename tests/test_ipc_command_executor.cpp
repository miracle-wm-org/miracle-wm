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

#include "ipc_command_executor.h"
#include "mock_command_controller.h"
#include "mock_launcher.h"
#include <gmock/gmock.h>

using namespace miracle;
using namespace testing;

class IpcCommandExecutorTest : public Test
{
public:
    IpcCommandExecutorTest() :
        executor(controller, launcher)
    {
    }

    std::shared_ptr<test::MockCommandController> controller = std::make_shared<test::MockCommandController>();
    std::shared_ptr<test::MockLauncher> launcher = std::make_shared<test::MockLauncher>();
    IpcCommandExecutor executor;
};

TEST_F(IpcCommandExecutorTest, ExecCommandWorksForSimpleCase)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::exec, "exec ls", {}, { "ls" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*launcher, launch(AllOf(Field(&StartupApp::command, Eq("ls ")), Field(&StartupApp::restart_on_death, Eq(false)), Field(&StartupApp::no_startup_id, Eq(false)), Field(&StartupApp::should_halt_compositor_on_death, Eq(false)), Field(&StartupApp::in_systemd_scope, Eq(false)))));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, ExecCommandWorksWithNoStartupIdFlag)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::exec, "exec ls",
        { "--no-startup-id" },
        { "ls" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*launcher, launch(AllOf(Field(&StartupApp::command, Eq("ls ")), Field(&StartupApp::restart_on_death, Eq(false)), Field(&StartupApp::no_startup_id, Eq(true)), Field(&StartupApp::should_halt_compositor_on_death, Eq(false)), Field(&StartupApp::in_systemd_scope, Eq(false)))));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, ExecCommandFailsIfArgumentsAreEmpty)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::exec, "exec ls", {}, {});
    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].error, Eq("No arguments were supplied"));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
}

TEST_F(IpcCommandExecutorTest, SplitCommandVerticalWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::split, "split vertical", {}, { "vertical" });

    EXPECT_CALL(*controller, try_request_vertical);

    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, SplitCommandHorizontalWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::split, "split vertical", {}, { "horizontal" });

    EXPECT_CALL(*controller, try_request_horizontal);

    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, SplitCommandToggleWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::split, "split vertical", {}, { "toggle" });

    EXPECT_CALL(*controller, try_toggle_layout);

    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, SplitCommandNoArgsResultsinError)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::split, "split vertical", {}, {});

    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
    EXPECT_THAT(validation_result[0].error, Eq("No arguments were supplied"));
}

TEST_F(IpcCommandExecutorTest, SplitCommandInvalidArgsResultsinError)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::split, "split vertical", {}, { "meow" });

    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
    EXPECT_THAT(validation_result[0].error, Eq("Unknown argument meow"));
}
