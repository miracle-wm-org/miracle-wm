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
#include "parent_container.h"
#include "policy.h"
#include "stub_configuration.h"
#include "vertical_display_configuration_policy.h"

#include <memory>
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

class PolicyTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    PolicyTest()
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

        // (mattkae): Arbitrary sleep so that we can account for the fact
        // that an initial pointer event will come through and attempt
        // to focus the first workspace.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
                std::make_shared<ConfigObserverRegistrar>(),
                miral::Magnifier());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 }, { 800, 600 } }
        });
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
                std::make_shared<ConfigObserverRegistrar>(),
                miral::Magnifier());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 },   { 800, 600 }  },
            mir::geometry::Rectangle { { 800, 0 }, { 1000, 600 } }
        });
    }

    std::shared_ptr<Config> config;
};

class TripleHorizontalWindowPolicyTest : public PolicyTest
{
public:
    TripleHorizontalWindowPolicyTest() :
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
                std::make_shared<ConfigObserverRegistrar>(),
                miral::Magnifier());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 },    { 800, 600 }  },
            mir::geometry::Rectangle { { 800, 0 },  { 1000, 600 } },
            mir::geometry::Rectangle { { 1800, 0 }, { 900, 600 }  }
        });
    }

    std::shared_ptr<Config> config;
};

TEST_F(DoubleWindowPolicyTest, DefaultWindowIsTilingWindow)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->first_tiling(), Ne(nullptr));
    EXPECT_THAT(compositor_state->first_tiling()->window(), Eq(window));
}

TEST_F(DoubleWindowPolicyTest, CanRemoveOutputWithContainersOnIt)
{
    // Setup: focus the first workspace before we begin
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const focus_payload = "workspace --no-auto-back-and-forth 2";
    uint32_t focus_payload_len = static_cast<uint32_t>(focus_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, focus_payload.c_str(), &focus_payload_len);

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

TEST_F(DoubleWindowPolicyTest, CanRemoveALlOutputsAndReAddOne)
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
    uint32_t response_len = static_cast<uint32_t>(payload.size());
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

TEST_F(SingleWindowPolicyTest, startup_ipc_command_runs_when_container_opens)
{
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const payload = "for_window [workspace=\"1\"] fullscreen toggle";
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, payload.c_str(), &payload_len);

    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(tools().info_for(window).state(), Eq(mir_window_state_fullscreen));
}

TEST_F(SingleWindowPolicyTest, MoveContainerToMark)
{
    // Open up the connection
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);

    // Open a first window and mark it
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const windowA = create_window(app, spec);
    std::string const mark_payload = "mark meow";
    uint32_t mark_payload_len = static_cast<uint32_t>(mark_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, mark_payload.c_str(), &mark_payload_len);

    // Open a second and third window
    auto const windowB = create_window(app, spec);
    auto const windowC = create_window(app, spec);

    // Move the third window to the mark
    std::string const move_payload = "move container to mark meow";
    uint32_t move_payload_len = static_cast<uint32_t>(move_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, move_payload.c_str(), &move_payload_len);

    // Assert that the third window is in the second position on its parent
    auto const container = compositor_state->focused_container();
    EXPECT_THAT(container->window(), Eq(windowC));
    EXPECT_THAT(container->get_parent().lock()->get_index_of_node(container), Eq(1));
}

struct LayoutSingleWindowPolicyData
{
    std::string layout_command;
    LayoutScheme scheme;
};

class LayoutSingleWindowPolicyTest : public SingleWindowPolicyTest, public WithParamInterface<LayoutSingleWindowPolicyData>
{
};

TEST_P(LayoutSingleWindowPolicyTest, LayoutCommandTest)
{
    auto const param = GetParam();

    // Open up the connection
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);

    // Open a first window and mark it
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const windowA = create_window(app, spec);
    std::string const split_payload = param.layout_command;
    uint32_t len = static_cast<uint32_t>(split_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, split_payload.c_str(), &len);

    auto const container = compositor_state->focused_container();
    EXPECT_THAT(container->get_parent().lock()->get_scheme(), Eq(param.scheme));
}

INSTANTIATE_TEST_SUITE_P(
    LayoutSingleWindowPolicyTest,
    LayoutSingleWindowPolicyTest,
    ::testing::Values(
        LayoutSingleWindowPolicyData { { "layout splitv" }, LayoutScheme::vertical },
        LayoutSingleWindowPolicyData { { "layout splith" }, LayoutScheme::horizontal },
        LayoutSingleWindowPolicyData { { "layout tabbed" }, LayoutScheme::tabbing },
        LayoutSingleWindowPolicyData { { "layout stacking" }, LayoutScheme::stacking },
        LayoutSingleWindowPolicyData { { "layout default" }, LayoutScheme::horizontal },
        LayoutSingleWindowPolicyData { { "layout splith; layout toggle split" }, LayoutScheme::vertical },
        LayoutSingleWindowPolicyData { { "layout splith; layout toggle splith tabbed" }, LayoutScheme::tabbing }));

TEST_F(DoubleWindowPolicyTest, MoveWorkspaceToNextOutput)
{
    // Setup: Create a new window and sanity check that it is on workspace 2
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const window = create_window(app, spec);
    auto const output_having_workspace_taken = compositor_state->focused_container()->get_output();
    EXPECT_THAT(compositor_state->focused_container()->get_workspace()->num(), Eq(2));

    // Act: Issue a command to move the workspace to the next output
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const payload = "move workspace to output next";
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, payload.c_str(), &payload_len);

    // Expect: that the current output has workspace 2 and the other output has workspace 1
    // (because it gets reassigned!)
    EXPECT_THAT(output_having_workspace_taken->get_workspaces()[0]->num(), Eq(1));
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_workspaces()[0]->num(), Eq(2));
}

TEST_F(TripleHorizontalWindowPolicyTest, MoveWorkspaceToLeftOutput)
{
    // Setup: focus the last workspace before we begin
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const focus_payload = "workspace --no-auto-back-and-forth 3";
    uint32_t focus_payload_len = static_cast<uint32_t>(focus_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, focus_payload.c_str(), &focus_payload_len);

    // Setup: Create a new window and sanity check that it is on workspace 3
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->focused_container()->get_workspace()->num(), Eq(3));

    // Act: Issue a command to move the workspace to the left output
    std::string const payload = "move workspace to output left";
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, payload.c_str(), &payload_len);

    // Expect: that the container is still on workspace 3 but output 2
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_workspaces()[0]->num(), Eq(3));
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_area(), Eq(mir::geometry::Rectangle {
                                                                                     { 800,  0   },
                                                                                     { 1000, 600 }
    }));
}

TEST_F(TripleHorizontalWindowPolicyTest, MoveWorkspaceToRightOutput)
{
    // Setup: focus the first workspace before we begin
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const focus_payload = "workspace --no-auto-back-and-forth 1";
    uint32_t focus_payload_len = static_cast<uint32_t>(focus_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, focus_payload.c_str(), &focus_payload_len);

    // Setup: Create a new window and sanity check that it is on workspace 1
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->focused_container()->get_workspace()->num(), Eq(1));

    // Act: Issue a command to move the workspace to the right output
    std::string const payload = "move workspace to output right";
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, payload.c_str(), &payload_len);

    // Expect: that the container is still on workspace 1 but output 2
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_workspaces()[0]->num(), Eq(1));
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_area(), Eq(mir::geometry::Rectangle {
                                                                                     { 800,  0   },
                                                                                     { 1000, 600 }
    }));
}

class DoubleVerticalWindowPolicyTest : public DoubleWindowPolicyTest
{
public:
    DoubleVerticalWindowPolicyTest() :
        DoubleWindowPolicyTest()
    {
        server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
                                                     -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return std::make_shared<test::VerticalDisplayConfigurationPolicy>();
        });
    }

protected:
    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 },   { 800, 600 } },
            mir::geometry::Rectangle { { 0, 600 }, { 800, 800 } }
        });
    }
};

TEST_F(DoubleVerticalWindowPolicyTest, MoveWorkspaceToUpOutput)
{
    // Setup: focus the first workspace before we begin
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const focus_payload = "workspace --no-auto-back-and-forth 2";
    uint32_t focus_payload_len = static_cast<uint32_t>(focus_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, focus_payload.c_str(), &focus_payload_len);

    // Setup: Create a new window and sanity check that it is on workspace 2
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->focused_container()->get_workspace()->num(), Eq(2));

    // Act: Issue a command to move the workspace to the up output
    std::string const payload = "move workspace to output up";
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, payload.c_str(), &payload_len);

    // Expect: that the container is still on workspace 2 but output 1
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_workspaces()[0]->num(), Eq(2));
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_area(), Eq(mir::geometry::Rectangle {
                                                                                     { 0,   0   },
                                                                                     { 800, 600 }
    }));
}

TEST_F(DoubleVerticalWindowPolicyTest, MoveWorkspaceToDownOutput)
{
    // Setup: focus the first workspace before we begin
    auto const socket_path = get_socketpath();
    auto const socket_fd = ipc_open_socket(socket_path);
    std::string const focus_payload = "workspace --no-auto-back-and-forth 1";
    uint32_t focus_payload_len = static_cast<uint32_t>(focus_payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, focus_payload.c_str(), &focus_payload_len);

    // Setup: Create a new window and sanity check that it is on workspace 1
    auto const app = open_application("test");
    miral::WindowSpecification const spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->focused_container()->get_workspace()->num(), Eq(1));

    // Act: Issue a command to move the workspace to the up output
    std::string const payload = "move workspace to output down";
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    ipc_single_command(socket_fd, IPC_COMMAND, payload.c_str(), &payload_len);

    // Expect: that the container is still on workspace 1 but output 2
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_workspaces()[0]->num(), Eq(1));
    EXPECT_THAT(compositor_state->focused_container()->get_output()->get_area(), Eq(mir::geometry::Rectangle {
                                                                                     { 0,   600 },
                                                                                     { 800, 800 }
    }));
}
