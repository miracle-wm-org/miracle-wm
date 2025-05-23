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

TEST_F(IpcCommandExecutorTest, LayoutCommandWithoutArgsResultsInError)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout", {}, {});
    parse_result.commands.push_back(command);

    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
    EXPECT_THAT(validation_result[0].error, Eq("No arguments were supplied"));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandDefaultWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout default", {}, { "default" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, set_layout_default);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandTabbedWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout tabbed", {}, { "tabbed" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, set_layout(LayoutScheme::tabbing, testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandStackingWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout stacking", {}, { "stacking" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, set_layout(LayoutScheme::stacking, testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandSplitvWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout splitv", {}, { "splitv" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, set_layout(LayoutScheme::vertical, testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandSplithWorks)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout splith", {}, { "splith" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, set_layout(LayoutScheme::horizontal, testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandToggleWithoutArgResultsInError)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout toggle", {}, { "toggle" });
    parse_result.commands.push_back(command);

    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
    EXPECT_THAT(validation_result[0].error, Eq("Expected argument after 'layout toggle ...'"));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandToggleWithSplit)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout toggle split", {}, { "toggle", "split" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, try_toggle_layout(false, testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandToggleWithAll)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout toggle all", {}, { "toggle", "all" });
    parse_result.commands.push_back(command);

    EXPECT_CALL(*controller, try_toggle_layout(true, testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandToggleWithCycle)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout toggle split tabbed stacking splitv splith", {}, { "toggle", "split", "tabbed", "stacking", "splitv", "splith" });
    parse_result.commands.push_back(command);
    EXPECT_CALL(*controller, try_cycle_through_request_types(ElementsAre(LayoutRequestType::split, LayoutRequestType::tabbed, LayoutRequestType::stacking, LayoutRequestType::splitv, LayoutRequestType::splith), testing::_));
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(true));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandToggleWithCycleErrorsWhenInvalid)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout toggle split tabbed stacking splitv splith unknown", {}, { "toggle", "split", "tabbed", "stacking", "splitv", "splith", "unknown" });
    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
    EXPECT_THAT(validation_result[0].error, Eq("Unknown layout type in 'layout toggle': unknown"));
}

TEST_F(IpcCommandExecutorTest, LayoutCommandWithInvalidArg)
{
    IpcParseResult parse_result;
    IpcCommand command(IpcCommandType::layout, "layout unknown", {}, { "unknown" });
    parse_result.commands.push_back(command);
    auto const validation_result = executor.process(parse_result);
    EXPECT_THAT(validation_result.size(), Eq(1));
    EXPECT_THAT(validation_result[0].success, Eq(false));
    EXPECT_THAT(validation_result[0].parse_error, Eq(true));
    EXPECT_THAT(validation_result[0].error, Eq("Expected default/tabbed/stacking/splitv/splith/toggle after 'layout'"));
}
