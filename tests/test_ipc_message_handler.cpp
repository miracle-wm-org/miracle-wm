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

#include "ipc_message_handler.h"
#include "mock_command_controller.h"
#include "mock_configuration.h"
#include "mock_ipc_command_executor.h"
#include "mock_plugin_manager.h"
#include "mock_window_controller.h"
#include "version.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace miracle;
using namespace testing;

class IpcMessageHandlerTest : public Test
{
public:
    IpcMessageHandlerTest() :
        ipc_command_executor(new test::MockIpcCommandExecutor),
        message_handler(
            command_controller,
            std::unique_ptr<test::MockIpcCommandExecutor>(ipc_command_executor),
            config,
            window_controller,
            plugin_manager)
    {
        ON_CALL(*window_controller, invoke_under_lock)
            .WillByDefault([](std::function<void()> const& f)
        { f(); });
    }

    std::shared_ptr<test::MockCommandController> command_controller = std::make_shared<NiceMock<test::MockCommandController>>();
    std::shared_ptr<test::MockConfig> config = std::make_shared<NiceMock<test::MockConfig>>();
    std::shared_ptr<NiceMock<test::MockWindowController>> window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
    std::shared_ptr<NiceMock<test::MockPluginManager>> plugin_manager = std::make_shared<NiceMock<test::MockPluginManager>>();
    test::MockIpcCommandExecutor* ipc_command_executor;
    IpcMessageHandler message_handler;
};

TEST_F(IpcMessageHandlerTest, CanRunIpcCommand)
{
    std::vector<IpcValidationResult> const validation_results = {
        IpcValidationResult::create_success()
    };
    std::string const command = "workspace 1";
    EXPECT_CALL(*ipc_command_executor, process(testing::_))
        .WillOnce(Return(validation_results));
    const char* payload = command.c_str();
    auto const result = message_handler.handle_msg(IpcType::IPC_COMMAND, payload, static_cast<uint32_t>(command.size()));
    EXPECT_THAT(result.type, Eq(IpcType::IPC_COMMAND));
    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json[0]["success"], Eq(true));
}

TEST_F(IpcMessageHandlerTest, CanFailIpcCommand)
{
    std::vector<IpcValidationResult> const validation_results = {
        IpcValidationResult::create_failure("failed", true)
    };
    std::string const command = "workspace 1";
    EXPECT_CALL(*ipc_command_executor, process(testing::_))
        .WillOnce(Return(validation_results));
    const char* payload = command.c_str();
    auto const result = message_handler.handle_msg(IpcType::IPC_COMMAND, payload, static_cast<uint32_t>(command.size()));
    EXPECT_THAT(result.type, Eq(IpcType::IPC_COMMAND));
    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json[0]["success"], Eq(false));
    EXPECT_THAT(result_json[0]["parse_error"], Eq(true));
    EXPECT_THAT(result_json[0]["error"], Eq("failed"));
}

TEST_F(IpcMessageHandlerTest, CanGetWorkspaces)
{
    nlohmann::json throwaway_json;
    EXPECT_CALL(*command_controller, workspaces_json)
        .WillOnce(Return(throwaway_json));
    const char* payload = "";
    message_handler.handle_msg(IpcType::IPC_GET_WORKSPACES, payload, 0);
}

TEST_F(IpcMessageHandlerTest, CanGetOutputs)
{
    nlohmann::json throwaway_json;
    EXPECT_CALL(*command_controller, outputs_json)
        .WillOnce(Return(throwaway_json));
    const char* payload = "";
    message_handler.handle_msg(IpcType::IPC_GET_OUTPUTS, payload, 0);
}

struct SubscriptionTestParams
{
    std::string const subscription;
    IpcType const expected;
};

class IpcMessageHandlerSubscriptionTest : public IpcMessageHandlerTest, public ::WithParamInterface<SubscriptionTestParams>
{
};

TEST_P(IpcMessageHandlerSubscriptionTest, CanSubcribeToEvent)
{
    auto param = GetParam();
    nlohmann::json subscription;
    subscription.push_back(param.subscription);

    auto const payload = to_string(subscription);
    auto const result = message_handler.handle_msg(IpcType::IPC_SUBSCRIBE, payload.c_str(), static_cast<uint32_t>(payload.size()));
    EXPECT_THAT(result.subscribed_events == ipc_event_mask(param.expected), Eq(true));
}

INSTANTIATE_TEST_SUITE_P(
    IpcMessageHandlerSubscriptionTest,
    IpcMessageHandlerSubscriptionTest,
    ::Values(
        SubscriptionTestParams("workspace", IpcType::IPC_EVENT_WORKSPACE),
        SubscriptionTestParams("window", IpcType::IPC_EVENT_WINDOW),
        SubscriptionTestParams("input", IpcType::IPC_EVENT_INPUT),
        SubscriptionTestParams("mode", IpcType::IPC_EVENT_MODE),
        SubscriptionTestParams("tick", IpcType::IPC_EVENT_TICK),
        SubscriptionTestParams("shutdown", IpcType::IPC_EVENT_SHUTDOWN),
        SubscriptionTestParams("binding", IpcType::IPC_EVENT_BINDING),
        SubscriptionTestParams("output", IpcType::IPC_EVENT_OUTPUT)));

TEST_F(IpcMessageHandlerTest, CanSubscribeToPluginNamespace)
{
    nlohmann::json subscription;
    subscription.push_back("my-plugin");

    auto const payload = to_string(subscription);
    auto const result = message_handler.handle_msg(IpcType::IPC_SUBSCRIBE, payload.c_str(), static_cast<uint32_t>(payload.size()));
    EXPECT_THAT(result.subscribed_events == ipc_event_mask(IpcType::IPC_EVENT_PLUGIN), Eq(true));
    EXPECT_THAT(result.subscribed_plugin_namespaces, ElementsAre("my-plugin"));
    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json["success"], Eq(true));
}

TEST_F(IpcMessageHandlerTest, CanFailToSubscribeToEvent)
{
    nlohmann::json subscription;
    subscription.push_back(42);

    auto const payload = to_string(subscription);
    auto const result = message_handler.handle_msg(IpcType::IPC_SUBSCRIBE, payload.c_str(), static_cast<uint32_t>(payload.size()));
    EXPECT_THAT(result.subscribed_events, Eq(0));
    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json["success"], Eq(false));
    EXPECT_THAT(result_json["error"], Eq("Invalid IPC subscription event: 42"));
}

TEST_F(IpcMessageHandlerTest, RoutesPluginCommandToPluginManager)
{
    EXPECT_CALL(*plugin_manager, handle_plugin_command("my-plugin", _))
        .WillOnce(Return(std::optional<std::string>(R"({"pong":true})")));

    nlohmann::json const request = {
        { "plugin",  "my-plugin"          },
        { "payload", { { "ping", true } } }
    };
    auto const payload = to_string(request);
    auto const result = message_handler.handle_msg(IpcType::IPC_PLUGIN_COMMAND, payload.c_str(), static_cast<uint32_t>(payload.size()));

    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json["success"], Eq(true));
    EXPECT_THAT(result_json["response"], Eq(nlohmann::json({
                                             { "pong", true }
    })));
}

TEST_F(IpcMessageHandlerTest, PluginCommandFailsWhenNoPluginRegistered)
{
    EXPECT_CALL(*plugin_manager, handle_plugin_command("my-plugin", _))
        .WillOnce(Return(std::nullopt));

    nlohmann::json const request = {
        { "plugin",  "my-plugin"          },
        { "payload", { { "ping", true } } }
    };
    auto const payload = to_string(request);
    auto const result = message_handler.handle_msg(IpcType::IPC_PLUGIN_COMMAND, payload.c_str(), static_cast<uint32_t>(payload.size()));

    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json["success"], Eq(false));
    EXPECT_THAT(result_json["error"], Eq("No plugin is registered for namespace: my-plugin"));
}

TEST_F(IpcMessageHandlerTest, CanGetTree)
{
    nlohmann::json throwaway_json;
    EXPECT_CALL(*command_controller, to_json)
        .WillOnce(Return(throwaway_json));
    const char* payload = "";
    auto result = message_handler.handle_msg(IpcType::IPC_GET_TREE, payload, 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_GET_TREE));
}

TEST_F(IpcMessageHandlerTest, CanGetVersionInfo)
{
    std::string filename = "config.yaml";
    EXPECT_CALL(*config, get_filename)
        .WillOnce(ReturnRef(filename));
    const char* payload = "";
    auto result = message_handler.handle_msg(IpcType::IPC_GET_VERSION, payload, 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_GET_VERSION));
    nlohmann::json result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json["major"], Eq(MIRACLE_WM_MAJOR));
    EXPECT_THAT(result_json["minor"], Eq(MIRACLE_WM_MINOR));
    EXPECT_THAT(result_json["patch"], Eq(MIRACLE_WM_PATCH));
    EXPECT_THAT(result_json["human_readable"], Eq(MIRACLE_VERSION_STRING));
    EXPECT_THAT(result_json["loaded_config_file_name"], Eq(filename));
}

TEST_F(IpcMessageHandlerTest, CanGetBindingModes)
{
    const char* payload = "";
    auto result = message_handler.handle_msg(IpcType::IPC_GET_BINDING_MODES, payload, 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_GET_BINDING_MODES));
    nlohmann::json const result_json = nlohmann::json::parse(result.payload);
    EXPECT_THAT(result_json, Eq(BINDING_MODE_STRINGS));
}

TEST_F(IpcMessageHandlerTest, CanGetBindingSstate)
{
    nlohmann::json throwaway_json;
    EXPECT_CALL(*command_controller, mode_to_json)
        .WillOnce(Return(throwaway_json));

    const char* payload = "";
    auto const result = message_handler.handle_msg(IpcType::IPC_GET_BINDING_STATE, payload, 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_GET_BINDING_STATE));
}

TEST_F(IpcMessageHandlerTest, CanSendTick)
{
    const char* payload = "";
    auto const result = message_handler.handle_msg(IpcType::IPC_SEND_TICK, payload, 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_SEND_TICK));
    EXPECT_THAT(result.payload, Eq("{\"success\": true}"));
    EXPECT_THAT(result.send_tick_event, Eq(true));
}

TEST_F(IpcMessageHandlerTest, CandSendIpcSync)
{
    const char* payload = "";
    auto const result = message_handler.handle_msg(IpcType::IPC_SYNC, payload, 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_SYNC));
    EXPECT_THAT(result.payload, Eq("{\"name\": \"default\"}"));
}

class UnsupportedIpcMessageHandlerTest : public IpcMessageHandlerTest, public WithParamInterface<IpcType>
{
};

TEST_P(UnsupportedIpcMessageHandlerTest, UnsupportedCommandsAreFatal)
{
    auto param = GetParam();
    const char* payload = "";
    auto const result = message_handler.handle_msg(param, payload, 0);
    EXPECT_THAT(result.type, Eq(param));
    EXPECT_THAT(result.fatal, Eq(true));
}

INSTANTIATE_TEST_SUITE_P(
    UnsupportedIpcMessageHandlerTest,
    UnsupportedIpcMessageHandlerTest,
    ::Values(
        IpcType::IPC_GET_BAR_CONFIG,
        IpcType::IPC_GET_CONFIG,
        IpcType::IPC_GET_INPUTS,
        IpcType::IPC_GET_SEATS));

TEST_F(IpcMessageHandlerTest, CanGetAllMark)
{
    std::unordered_set<std::string> marks = { "first", "second" };
    EXPECT_CALL(*command_controller, get_all_marks)
        .WillOnce(Return(marks));
    auto const result = message_handler.handle_msg(IpcType::IPC_GET_MARKS, "", 0);
    EXPECT_THAT(result.type, Eq(IpcType::IPC_GET_MARKS));
    nlohmann::json const result_json = nlohmann::json::parse(result.payload);

    std::unordered_set<std::string> result_marks;
    for (auto const& mark : result_json)
        result_marks.insert(mark);
    EXPECT_THAT(result_marks, Eq(marks));
}
