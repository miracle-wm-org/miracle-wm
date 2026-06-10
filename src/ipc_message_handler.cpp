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

#define MIR_LOG_COMPONENT "ipc_message_handler"

#include "ipc_message_handler.h"
#include "command_controller.h"
#include "config.h"
#include "ipc_command_executor.h"
#include "version.h"
#include "window_controller.h"
#include <mir/log.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace miracle;

IpcMessageHandler::IpcMessageHandler(std::shared_ptr<AbstractCommandController> const& command_controller,
    std::shared_ptr<AbstractIpcCommandExecutor> const& ipc_command_executor,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller) :
    command_controller { command_controller },
    ipc_command_executor { ipc_command_executor },
    config { config },
    window_controller { window_controller }
{
}

MessageHandlerResult
IpcMessageHandler::handle_msg(
    IpcType payload_type,
    const char* payload,
    uint32_t payload_length)
{
    MessageHandlerResult result;
    window_controller->invoke_under_lock([&]()
    {
        result = process_msg(payload_type, payload, payload_length);
    });
    return result;
}

MessageHandlerResult IpcMessageHandler::process_msg(
    IpcType payload_type,
    const char* payload,
    uint32_t)
{
    switch (payload_type)
    {
    case IpcType::IPC_COMMAND:
    {
        mir::log_debug("Processing miracle command: %s", payload);
        auto const result = process_ipc_command(payload);
        json j = json::array();
        for (auto const& command_result : result)
        {
            if (command_result.success)
            {
                j.push_back({
                    { "success", true }
                });
            }
            else
            {
                j.push_back({
                    { "success",     false                      },
                    { "parse_error", command_result.parse_error },
                    { "error",       command_result.error       },
                });
            }
        }
        return {
            .type = payload_type,
            .payload = to_string(j)
        };
    }
    case IpcType::IPC_GET_WORKSPACES:
    {
        auto json_string = to_string(command_controller->workspaces_json());
        return {
            .type = payload_type,
            .payload = json_string
        };
    }
    case IpcType::IPC_GET_OUTPUTS:
    {
        auto json_string = to_string(command_controller->outputs_json());
        return {
            .type = payload_type,
            .payload = json_string
        };
    }
    case IpcType::IPC_SUBSCRIBE:
    {
        json j = json::parse(payload);
        bool success = true;
        std::string error;
        MessageHandlerResult result = { .type = payload_type };
        for (auto const& i : j)
        {
            auto const event_type = i.template get<std::string>();
            mir::log_debug("Received subscription request from IPC client for event: %s", event_type.c_str());
            if (event_type == "workspace")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_WORKSPACE);
            else if (event_type == "output")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_OUTPUT);
            else if (event_type == "mode")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_MODE);
            else if (event_type == "window")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_WINDOW);
            else if (event_type == "binding")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_BINDING);
            else if (event_type == "shutdown")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_SHUTDOWN);
            else if (event_type == "tick")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_TICK);
            else if (event_type == "input")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_INPUT);
            else if (event_type == "config_errors")
                result.subscribed_events |= ipc_event_mask(IpcType::IPC_EVENT_CONFIG_ERRORS);
            else
            {
                mir::log_warning("Cannot process IPC subscription event for event_type: %s", event_type.c_str());
                error = "Invalid IPC subscription event: " + event_type;
                success = false;
                break;
            }
        }

        if (success)
        {
            result.payload = "{\"success\": true}";
        }
        else
        {
            json const response = {
                { "success", false },
                { "error",   error }
            };
            result.subscribed_events = 0;
            result.payload = to_string(response);
        }
        return result;
    }
    case IpcType::IPC_GET_TREE:
    {
        return {
            .type = payload_type,
            .payload = to_string(command_controller->to_json())
        };
    }
    case IpcType::IPC_GET_MARKS:
    {
        auto const marks = command_controller->get_all_marks();
        json j = json::array();
        for (auto const& mark : marks)
            j.push_back(mark);

        return {
            .type = payload_type,
            .payload = to_string(j)
        };
    }
    case IpcType::IPC_GET_VERSION:
    {
        json response = {
            { "major",                   MIRACLE_WM_MAJOR       },
            { "minor",                   MIRACLE_WM_MINOR       },
            { "patch",                   MIRACLE_WM_PATCH       },
            { "human_readable",          MIRACLE_VERSION_STRING },
            { "loaded_config_file_name", config->get_filename() }
        };
        return {
            .type = payload_type,
            .payload = to_string(response)
        };
    }
    case IpcType::IPC_GET_BINDING_MODES:
    {
        json response;
        for (auto const& str : BINDING_MODE_STRINGS)
            response.push_back(str);
        return {
            .type = payload_type,
            .payload = to_string(response)
        };
    }
    case IpcType::IPC_GET_BINDING_STATE:
    {
        return {
            .type = payload_type,
            .payload = to_string(command_controller->mode_to_json())
        };
    }
    case IpcType::IPC_SEND_TICK:
    {
        return {
            .type = payload_type,
            .payload = "{\"success\": true}",
            .send_tick_event = true
        };
    }
    case IpcType::IPC_SYNC:
    {
        return {
            .type = payload_type,
            .payload = "{\"name\": \"default\"}",
        };
    }
    default:
        mir::log_warning("Unknown payload type: %d", static_cast<int>(payload_type));
        return {
            .fatal = true,
            .type = payload_type
        };
    }
}

std::vector<IpcValidationResult> IpcMessageHandler::process_ipc_command(const char* command)
{
    IpcCommandParser parser(command);
    auto const pending_commands = parser.parse();
    return ipc_command_executor->process(pending_commands);
}
