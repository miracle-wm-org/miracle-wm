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

#include "config_observer.h"
#include "display_config.h"
#include "ipc_client.h"
#include "output_listener.h"
#include "policy.h"
#include "stub_configuration.h"

#include <mir/graphics/default_display_configuration_policy.h>
#include <mir_test_framework/window_management_test_harness.h>

#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>

namespace mg = mir::graphics;

using namespace testing;
using namespace miracle;

namespace
{
int argc = 1;
const char* argv[] = { "miracle-wm" };
}

class PolicyTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    explicit PolicyTest()
    {
        server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
                                                     -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return std::make_shared<mir::graphics::SideBySideDisplayConfigurationPolicy>();
        });
    }

    void SetUp() override
    {
        launcher(server);
        WindowManagementTestHarness::SetUp();
    }

    miral::ExternalClientLauncher launcher;
    std::shared_ptr<CompositorState> compositor_state = std::make_shared<CompositorState>();
};

class SingleWindowPolicyTest : public PolicyTest
{
public:
    SingleWindowPolicyTest() :
        config_path { std::filesystem::temp_directory_path() / "policy_test.yaml" },
        config { std::make_shared<FilesystemConfiguration>(
            registrar,
            config_path,
            true) }
    {
    }

    ~SingleWindowPolicyTest() override
    {
        std::filesystem::remove(config_path);
    }

    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<Policy>(
                tools,
                server,
                launcher,
                config,
                compositor_state,
                std::make_shared<OutputListenerMultiplexer>(),
                std::make_shared<DisplayConfig>(),
                std::make_shared<ConfigObserverRegistrar>());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        auto r = output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 }, { 800, 600 } }
        });
        return r;
    }

    std::string config_path;
    std::shared_ptr<ConfigObserverRegistrar> registrar = std::make_shared<ConfigObserverRegistrar>();
    std::shared_ptr<Config> config;
};

class DoubleWindowPolicyTest : public PolicyTest
{
public:
    DoubleWindowPolicyTest() :
        config { std::make_shared<test::StubConfiguration>() }
    {
    }

    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<Policy>(
                tools,
                server,
                launcher,
                config,
                compositor_state,
                std::make_shared<OutputListenerMultiplexer>(),
                std::make_shared<DisplayConfig>(),
                std::make_shared<ConfigObserverRegistrar>());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        auto r = output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 },   { 800, 600 }  },
            mir::geometry::Rectangle { { 800, 0 }, { 1000, 600 } }
        });
        return r;
    }

    std::shared_ptr<Config> config;
};

TEST_F(DoubleWindowPolicyTest, default_window_is_tiling_window)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->first_tiling(), Ne(nullptr));
    EXPECT_THAT(compositor_state->first_tiling()->window(), Eq(window));
}

TEST_F(DoubleWindowPolicyTest, can_remove_output_with_containers_open_on_it)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    auto const container = compositor_state->containers()[0].lock();
    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 800,  0   },
                                                   { 1000, 600 }
    }));
    update_outputs(output_configs_from_output_rectangles({
        mir::geometry::Rectangle { { 0, 0 }, { 800, 600 } }
    }));

    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 0,   0   },
                                                   { 800, 600 }
    }));

    update_outputs(output_configs_from_output_rectangles({
        mir::geometry::Rectangle { { 0, 0 },   { 800, 600 }  },
        mir::geometry::Rectangle { { 800, 0 }, { 1000, 600 } }
    }));

    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 0,   0   },
                                                   { 800, 600 }
    }));
}

TEST_F(DoubleWindowPolicyTest, can_remove_all_outputs_and_readd_it)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    auto const container = compositor_state->containers()[0].lock();
    update_outputs(output_configs_from_output_rectangles({}));

    update_outputs(output_configs_from_output_rectangles({
        mir::geometry::Rectangle { { 0, 0 }, { 400, 300 } }
    }));

    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 0,   0   },
                                                   { 400, 300 }
    }));
}

TEST_F(SingleWindowPolicyTest, can_move_container_to_workspace_that_doesnt_have_containers)
{
    {
        // Move to workspace 1
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action { mir_keyboard_action_down };
        xkb_keysym_t const keysym { 0 };
        int const scan_code { KEY_1 };
        MirInputEventModifiers const modifiers { mir_input_event_modifier_meta };
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window1 = create_window(app, spec);

    {
        // Move to workspace 2
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action { mir_keyboard_action_down };
        xkb_keysym_t const keysym { 0 };
        int const scan_code { KEY_2 };
        MirInputEventModifiers const modifiers { mir_input_event_modifier_meta };
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    auto const window2 = create_window(app, spec);

    {
        // Move to workspace 1
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action { mir_keyboard_action_down };
        xkb_keysym_t const keysym { 0 };
        int const scan_code { KEY_2 };
        MirInputEventModifiers const modifiers { mir_input_event_modifier_meta };
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    EXPECT_THAT(compositor_state->focused_container(), Eq(compositor_state->containers().front().lock()));

    {
        // Move the window1 to workspace 2
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action { mir_keyboard_action_down };
        xkb_keysym_t const keysym { 0 };
        int const scan_code { KEY_2 };
        MirInputEventModifiers const modifiers { mir_input_event_modifier_meta | mir_input_event_modifier_shift };
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    // Expect that all containers are on workspace 2
    for (auto const& container : compositor_state->containers())
    {
        EXPECT_THAT(container.lock()->get_workspace()->num(), Eq(2));
    }
}

TEST_F(SingleWindowPolicyTest, can_open_ipc_client)
{
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    EXPECT_THAT(socket_fd, Ne(-1));
}

TEST_F(SingleWindowPolicyTest, ipc_client_notified_on_binding_event)
{
    // Setup: Open a client and subscribe to binding events
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    nlohmann::json payload_json;
    payload_json.push_back("binding");
    auto const payload = to_string(payload_json);
    uint32_t response_len = payload.size();
    ipc_single_command(socket_fd, IPC_SUBSCRIBE, payload.c_str(), &response_len);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto thread = std::thread([&]()
    {
        ipc_response* reply = ipc_recv_response(socket_fd);
        EXPECT_THAT(reply, Ne(nullptr));

        std::string json_str;
        json_str.assign(reply->payload, reply->size);

        nlohmann::json json = nlohmann::json::parse(json_str);
        EXPECT_THAT(json["change"], Eq("run"));
        EXPECT_THAT(json["binding"]["command"], Eq("select_workspace_2"));
        EXPECT_THAT(json["binding"]["event_state_mask"][0], Eq("meta"));
        EXPECT_THAT(json["binding"]["input_code"], Eq(XKB_KEY_2));
        EXPECT_THAT(json["binding"]["symbol"], Eq("2"));
        EXPECT_THAT(json["binding"]["type"], Eq("keyboard"));
    });

    // Action: issue a key command (e.g. move to workspace 2)
    {
        // Move to workspace 2
        std::chrono::nanoseconds const event_timestamp = std::chrono::system_clock::now().time_since_epoch();
        MirKeyboardAction const action { mir_keyboard_action_down };
        xkb_keysym_t const keysym { XKB_KEY_2 };
        int const scan_code { KEY_2 };
        MirInputEventModifiers const modifiers { mir_input_event_modifier_meta };
        auto const event = mir::events::make_key_event(
            mir_input_event_type_key,
            event_timestamp,
            action,
            keysym,
            scan_code,
            modifiers);
        publish_event(*event);
    }

    thread.join();
}
