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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include <mir/version.h>
#include <miracle/config.h>
#include <miracle/cpp/config-cpp.h>

using namespace testing;

class CAPIWrapperTest : public Test
{
protected:
    void SetUp() override
    {
        config = std::make_unique<miracle::ConfigData>();
        wrapper = std::make_unique<miracle_config_load_result_t>();
        wrapper->config._internal = config.get();
    }

    std::unique_ptr<miracle::ConfigData> config;
    std::unique_ptr<miracle_config_load_result_t> wrapper;
};

TEST_F(CAPIWrapperTest, CanModifyIncludes)
{
    miracle_config_add_include(&wrapper->config, "/home/hi", 0);
    miracle_config_add_include(&wrapper->config, "/home/bye", 1);
    EXPECT_THAT(miracle_config_get_num_includes(&wrapper->config), Eq(2));
    EXPECT_STREQ(miracle_config_get_include(&wrapper->config, 0), "/home/hi");
    EXPECT_STREQ(miracle_config_get_include(&wrapper->config, 1), "/home/bye");
    miracle_config_set_include(&wrapper->config, "/home/meow", 1);
    EXPECT_STREQ(miracle_config_get_include(&wrapper->config, 1), "/home/meow");
    miracle_config_remove_include(&wrapper->config, 0);
    EXPECT_THAT(miracle_config_get_num_includes(&wrapper->config), Eq(1));
}

TEST_F(CAPIWrapperTest, CanSetValidPrimaryModifier)
{
    uint modifier = mir_input_event_modifier_alt;
    miracle_config_set_primary_modifier(&wrapper->config, modifier);
    EXPECT_EQ(miracle_config_get_primary_modifier(&wrapper->config), modifier);
}

TEST_F(CAPIWrapperTest, CannotSetInvalidPrimaryModifier)
{
    uint modifier = 123;
    miracle_config_set_primary_modifier(&wrapper->config, modifier);
    EXPECT_NE(miracle_config_get_primary_modifier(&wrapper->config), modifier);
}

TEST_F(CAPIWrapperTest, ModifierOptionsCount)
{
    ASSERT_THAT(miracle_config_get_modifier_options_count(), Eq(18));
}

TEST_F(CAPIWrapperTest, ModifierOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_modifier_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_modifier_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, MouseButtonOptionsCount)
{
    ASSERT_THAT(miracle_config_get_mouse_button_options_count(), Eq(8));
}

TEST_F(CAPIWrapperTest, MouseButtonOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_mouse_button_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_mouse_button_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, MouseActionsOptionsCount)
{
    ASSERT_THAT(miracle_config_get_mouse_actions_options_count(), Eq(mir_pointer_actions));
}

TEST_F(CAPIWrapperTest, MouseActionsOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_mouse_actions_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_mouse_actions_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, KeyboardActionsOptionsCount)
{
    ASSERT_THAT(miracle_config_get_keyboard_actions_options_count(), Eq(mir_keyboard_actions - 1));
}

TEST_F(CAPIWrapperTest, KeyboardActionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_keyboard_actions_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_keyboard_actions_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, BultInKeyboardCommandsCount)
{
    ASSERT_THAT(miracle_config_get_built_in_key_command_options_count(), Eq(static_cast<uint>(miracle::DefaultKeyCommand::MAX)));
}

TEST_F(CAPIWrapperTest, BultInKeyboardCommandsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_built_in_key_command_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_built_in_key_command_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, BuiltInAnimationTypeOptionsCount)
{
    ASSERT_THAT(miracle_config_get_built_in_animation_type_options_count(), Eq(static_cast<uint>(miracle::BultInAnimationType::max)));
}

TEST_F(CAPIWrapperTest, BuiltInAnimationTypeOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_built_in_animation_type_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_built_in_animation_type_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, EaseFunctionOptionsCount)
{
    ASSERT_THAT(miracle_config_get_ease_function_options_count(), Eq(static_cast<uint>(miracle::EaseFunction::max)));
}

TEST_F(CAPIWrapperTest, EaseFunctionOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_ease_function_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_ease_function_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, ContainerLayoutOptionsCount)
{
    ASSERT_THAT(miracle_config_get_layout_options_count(), Eq(static_cast<uint>(miracle::WindowLayoutStrategy::max)));
}

TEST_F(CAPIWrapperTest, ContainerLayoutOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_layout_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_layout_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, CanSetPrimaryButton)
{
    uint primary_button = mir_pointer_button_tertiary;
    miracle_config_set_primary_button(&wrapper->config, primary_button);
    EXPECT_THAT(miracle_config_get_primary_button(&wrapper->config), Eq(primary_button));
}

TEST_F(CAPIWrapperTest, CannotSetInvalidPrimaryButton)
{
    uint primary_button = 123;
    miracle_config_set_primary_button(&wrapper->config, primary_button);
    EXPECT_THAT(miracle_config_get_primary_button(&wrapper->config), Ne(primary_button));
}

TEST_F(CAPIWrapperTest, CanSetInnerGapsX)
{
    int const x = 100;
    miracle_config_set_inner_gaps_x(&wrapper->config, x);
    EXPECT_THAT(miracle_config_get_inner_gaps_x(&wrapper->config), Eq(x));
}

TEST_F(CAPIWrapperTest, CanSetInnerGapsY)
{
    int const y = 100;
    miracle_config_set_inner_gaps_y(&wrapper->config, y);
    EXPECT_THAT(miracle_config_get_inner_gaps_y(&wrapper->config), Eq(y));
}

TEST_F(CAPIWrapperTest, CanSetOuterGapsX)
{
    int const x = 100;
    miracle_config_set_outer_gaps_x(&wrapper->config, x);
    EXPECT_THAT(miracle_config_get_outer_gaps_x(&wrapper->config), Eq(x));
}

TEST_F(CAPIWrapperTest, CanSetOuterGapsY)
{
    int const y = 100;
    miracle_config_set_outer_gaps_y(&wrapper->config, y);
    EXPECT_THAT(miracle_config_get_outer_gaps_y(&wrapper->config), Eq(y));
}

TEST_F(CAPIWrapperTest, CanSetResizeJump)
{
    int const resize_jump = 100;
    miracle_config_set_resize_jump(&wrapper->config, resize_jump);
    EXPECT_THAT(miracle_config_get_resize_jump(&wrapper->config), Eq(resize_jump));
}

TEST_F(CAPIWrapperTest, CanSetAnimationsEnabled)
{
    miracle_config_set_animations_enabled(&wrapper->config, false);
    EXPECT_THAT(miracle_config_get_animations_enabled(&wrapper->config), Eq(false));
}

TEST_F(CAPIWrapperTest, CanSetTerminal)
{
    miracle_config_set_terminal(&wrapper->config, "meow");
    EXPECT_THAT(std::string(miracle_config_get_terminal(&wrapper->config)), Eq("meow"));
}

TEST_F(CAPIWrapperTest, CanSetNullTerminal)
{
    miracle_config_set_terminal(&wrapper->config, nullptr);
    EXPECT_THAT(miracle_config_get_terminal(&wrapper->config), Eq(nullptr));
}

TEST_F(CAPIWrapperTest, CanAddCustomKeyCommand)
{
    miracle_custom_key_command_t key_command = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command"
    };
    miracle_config_add_custom_key_command(
        &wrapper->config,
        &key_command);

    EXPECT_EQ(miracle_config_get_custom_key_command_count(&wrapper->config), 1);

    auto cmd = miracle_config_get_custom_key_command(&wrapper->config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_down);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_meta);
    EXPECT_EQ(cmd.key, 10);
    EXPECT_STREQ(cmd.command, "test-command");
}

TEST_F(CAPIWrapperTest, CanEditCustomKeyCommand)
{
    miracle_custom_key_command_t key_command = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command"
    };
    miracle_config_add_custom_key_command(
        &wrapper->config, &key_command);

    key_command = {
        mir_keyboard_action_up,
        mir_input_event_modifier_alt,
        12,
        "test-command-2"
    };
    miracle_config_edit_custom_key_command(
        &wrapper->config,
        0, &key_command);

    auto cmd = miracle_config_get_custom_key_command(&wrapper->config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_up);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_alt);
    EXPECT_EQ(cmd.key, 12);
    EXPECT_STREQ(cmd.command, "test-command-2");
}

TEST_F(CAPIWrapperTest, CannotEditCustomKeyCommandWithIndexGreaterThanRange)
{
    miracle_custom_key_command_t key_command = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command"
    };
    miracle_config_add_custom_key_command(
        &wrapper->config,
        &key_command);

    key_command = {
        mir_keyboard_action_up,
        mir_input_event_modifier_alt,
        12,
        "test-command-2"
    };
    miracle_config_edit_custom_key_command(
        &wrapper->config,
        100000,
        &key_command);

    auto cmd = miracle_config_get_custom_key_command(&wrapper->config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_down);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_meta);
    EXPECT_EQ(cmd.key, 10);
    EXPECT_STREQ(cmd.command, "test-command");
}

TEST_F(CAPIWrapperTest, CanRemoveCustomKeyCommand)
{
    miracle_custom_key_command_t key_command = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command"
    };
    miracle_config_add_custom_key_command(
        &wrapper->config,
        &key_command);

    EXPECT_TRUE(miracle_config_remove_custom_key_command(&wrapper->config, 0));
    EXPECT_EQ(miracle_config_get_custom_key_command_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, CanAddBuiltInKeyCommand)
{
    miracle_built_in_key_command_override_t override = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen)
    };
    miracle_config_add_built_in_key_command_override(
        &wrapper->config,
        &override);
    EXPECT_THAT(miracle_config_get_built_in_key_command_override_count(&wrapper->config), Eq(1));
    auto const key_command = miracle_config_get_built_in_key_command_override(
        &wrapper->config, 0);
    EXPECT_THAT(key_command.action, Eq(mir_keyboard_action_down));
    EXPECT_THAT(key_command.modifiers, Eq(mir_input_event_modifier_meta));
    EXPECT_THAT(key_command.key, Eq(KEY_F));
    EXPECT_THAT(key_command.command, Eq(static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen)));
}

TEST_F(CAPIWrapperTest, CanUpdateBuiltInKeyCommand)
{
    miracle_built_in_key_command_override_t override = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen)
    };
    miracle_config_add_built_in_key_command_override(
        &wrapper->config,
        &override);

    override = {
        mir_keyboard_action_up,
        mir_input_event_modifier_alt,
        KEY_K,
        static_cast<uint>(miracle::DefaultKeyCommand::MoveDown)
    };
    miracle_config_set_built_in_key_command_override(
        &wrapper->config,
        0,
        &override);

    auto const key_command = miracle_config_get_built_in_key_command_override(
        &wrapper->config, 0);
    EXPECT_THAT(key_command.action, Eq(mir_keyboard_action_up));
    EXPECT_THAT(key_command.modifiers, Eq(mir_input_event_modifier_alt));
    EXPECT_THAT(key_command.key, Eq(KEY_K));
    EXPECT_THAT(key_command.command, Eq(static_cast<uint>(miracle::DefaultKeyCommand::MoveDown)));
}

TEST_F(CAPIWrapperTest, CanRemoveBuiltInKeyCommand)
{
    miracle_built_in_key_command_override_t override = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen)
    };
    miracle_config_add_built_in_key_command_override(
        &wrapper->config,
        &override);

    EXPECT_THAT(miracle_config_remove_built_in_key_command_override(&wrapper->config, 0), Eq(true));
}

TEST_F(CAPIWrapperTest, CanAddStartupApps)
{
    miracle_startup_app_t startup_app = {
        "test-app",
        true,
        false,
        true,
        false
    };
    miracle_config_add_startup_app(
        &wrapper->config,
        &startup_app);

    EXPECT_EQ(miracle_config_get_startup_app_count(&wrapper->config), 1);

    auto app = miracle_config_get_startup_app(&wrapper->config, 0);
    EXPECT_STREQ(app.command, "test-app");
    EXPECT_TRUE(app.restart_on_death);
    EXPECT_FALSE(app.no_startup_id);
    EXPECT_TRUE(app.should_halt_compositor_on_death);
    EXPECT_FALSE(app.in_systemd_scope);
}

TEST_F(CAPIWrapperTest, CanSetStartupApps)
{
    miracle_startup_app_t startup_app = {
        "test-app",
        true,
        false,
        true,
        false
    };
    miracle_config_add_startup_app(
        &wrapper->config,
        &startup_app);

    startup_app = {
        "test-app-2",
        false,
        true,
        false,
        true
    };
    miracle_config_set_startup_app(
        &wrapper->config,
        0,
        &startup_app);

    auto app = miracle_config_get_startup_app(&wrapper->config, 0);
    EXPECT_STREQ(app.command, "test-app-2");
    EXPECT_FALSE(app.restart_on_death);
    EXPECT_TRUE(app.no_startup_id);
    EXPECT_FALSE(app.should_halt_compositor_on_death);
    EXPECT_TRUE(app.in_systemd_scope);
}

TEST_F(CAPIWrapperTest, CanRemoveStartupApps)
{
    miracle_startup_app_t startup_app = {
        "test-app",
        true,
        false,
        true,
        false
    };
    miracle_config_add_startup_app(
        &wrapper->config,
        &startup_app);

    miracle_config_remove_startup_app(&wrapper->config, 0);
    EXPECT_EQ(miracle_config_get_startup_app_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, CanAddEnvironmentVariable)
{
    miracle_environment_variable_t variable = { "first", "second" };
    miracle_config_add_environment_variable(
        &wrapper->config,
        &variable);

    EXPECT_THAT(miracle_config_get_environment_variable_count(&wrapper->config), Eq(1));
    auto const env_variable = miracle_config_get_environment_variable(&wrapper->config, 0);
    EXPECT_THAT(std::string(env_variable.key), Eq("first"));
    EXPECT_THAT(std::string(env_variable.value), Eq("second"));
}

TEST_F(CAPIWrapperTest, CanSetEnvironmentVariable)
{
    miracle_environment_variable_t variable = { "first", "second" };
    miracle_config_add_environment_variable(
        &wrapper->config,
        &variable);

    variable = { "third", "fourth" };
    miracle_config_set_environment_variable(
        &wrapper->config,
        0,
        &variable);

    auto const env_variable = miracle_config_get_environment_variable(&wrapper->config, 0);
    EXPECT_THAT(std::string(env_variable.key), Eq("third"));
    EXPECT_THAT(std::string(env_variable.value), Eq("fourth"));
}

TEST_F(CAPIWrapperTest, CanRemoveEnvironmentVariable)
{
    miracle_environment_variable_t variable = { "first", "second" };
    miracle_config_add_environment_variable(
        &wrapper->config,
        &variable);
    miracle_config_remove_environment_variable(&wrapper->config, 0);

    EXPECT_THAT(miracle_config_get_environment_variable_count(&wrapper->config), Eq(0));
}

TEST_F(CAPIWrapperTest, BorderConfig)
{
    miracle_border_config_t border_config = {
        2,
        3.f,
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f, 0.5f, 0.5f, 1.0f }
    };
    miracle_config_set_border_config(
        &wrapper->config,
        &border_config);

    auto border = miracle_config_get_border_config(&wrapper->config);
    EXPECT_EQ(border.size, 2);
    EXPECT_THAT(border.radius, Eq(3.f));
    EXPECT_THAT(border.focus_color, ElementsAre(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_THAT(border.color, ElementsAre(0.5f, 0.5f, 0.5f, 1.0f));
}

TEST_F(CAPIWrapperTest, CanSetAnimationDefinitions)
{
    auto animateable_event = miracle_config_get_animateable_event(
        &wrapper->config,
        0);
    animateable_event.duration_seconds = 0.5f;
    miracle_config_set_animateable_event(&wrapper->config, 0, &animateable_event);
    auto result = miracle_config_get_animateable_event(
        &wrapper->config,
        0);

    EXPECT_FLOAT_EQ(result.duration_seconds, 0.5f);
    EXPECT_EQ(result.num_parts, 1);
    EXPECT_FALSE(result.is_default);
}

TEST_F(CAPIWrapperTest, CanSetAnimationPlugin)
{
    auto animateable_event = miracle_config_get_animateable_event(
        &wrapper->config,
        0);
    animateable_event.duration_seconds = 0.5f;
    animateable_event.type = static_cast<uint>(miracle::AnimationType::plugin);
    animateable_event.plugin_name = "plugin";
    miracle_config_set_animateable_event(&wrapper->config, 0, &animateable_event);
    auto result = miracle_config_get_animateable_event(
        &wrapper->config,
        0);

    EXPECT_FLOAT_EQ(result.duration_seconds, 0.5f);
    EXPECT_EQ(result.type, static_cast<uint>(miracle::AnimationType::plugin));
    EXPECT_EQ(std::string(result.plugin_name), "plugin");
    EXPECT_EQ(result.num_parts, 0);
    EXPECT_EQ(result.duration_seconds, 0.5f);
}

TEST_F(CAPIWrapperTest, CanResetAnimationDefinitions)
{
    auto animateable_event = miracle_config_get_animateable_event(
        &wrapper->config,
        0);
    animateable_event.duration_seconds = 0.5f;
    miracle_config_set_animateable_event(&wrapper->config, 0, &animateable_event);
    miracle_config_set_animateable_event(
        &wrapper->config,
        0,
        &animateable_event);
    miracle_config_reset_animation_definition(
        &wrapper->config,
        0);
    auto result = miracle_config_get_animateable_event(
        &wrapper->config,
        0);

    EXPECT_TRUE(result.is_default);
}

TEST_F(CAPIWrapperTest, CanAddWorkspaceConfig)
{
    miracle_workspace_config_t workspace_config = {
        true, // has_num
        1, // num
        false, // has_name
        nullptr, // name
        true, // has_layout_strategy
        static_cast<int>(miracle::WindowLayoutStrategy::floating)
    };
    miracle_config_add_workspace_config(
        &wrapper->config,
        &workspace_config);

    EXPECT_EQ(miracle_config_get_workspace_config_count(&wrapper->config), 1);

    auto ws = miracle_config_get_workspace_config(&wrapper->config, 0);
    EXPECT_TRUE(ws.has_num);
    EXPECT_EQ(ws.num, 1);
    EXPECT_TRUE(ws.has_layout_strategy);
    EXPECT_EQ(ws.layout_strategy, static_cast<int>(miracle::WindowLayoutStrategy::floating));
}

TEST_F(CAPIWrapperTest, CanSetWorkspaceConfig)
{
    miracle_workspace_config_t workspace_config = {
        true, // has_num
        1, // num
        false, // has_name
        nullptr, // name
        true, // has_layout_strategy
        static_cast<int>(miracle::WindowLayoutStrategy::floating)
    };
    miracle_config_add_workspace_config(
        &wrapper->config,
        &workspace_config);

    workspace_config = {
        true, // has_num
        2, // num
        true, // has_name
        "Other", // name
        true, // has_layout_strategy
        static_cast<int>(miracle::WindowLayoutStrategy::floating)
    };
    miracle_config_set_workspace_config(
        &wrapper->config,
        0,
        &workspace_config);

    auto ws = miracle_config_get_workspace_config(&wrapper->config, 0);
    EXPECT_TRUE(ws.has_num);
    EXPECT_EQ(ws.num, 2);
    EXPECT_TRUE(ws.has_name);
    EXPECT_STREQ(ws.name, "Other");
    EXPECT_TRUE(ws.has_layout_strategy);
    EXPECT_EQ(ws.layout_strategy, static_cast<int>(miracle::WindowLayoutStrategy::floating));
}

TEST_F(CAPIWrapperTest, CanRemoveWorkspaceConfig)
{
    miracle_workspace_config_t workspace_config = {
        true, // has_num
        1, // num
        false, // has_name
        nullptr, // name
        true, // has_layout_strategy
        static_cast<int>(miracle::WindowLayoutStrategy::floating)
    };
    miracle_config_add_workspace_config(
        &wrapper->config,
        &workspace_config);

    miracle_config_remove_workspace_config(&wrapper->config, 0);
    EXPECT_EQ(miracle_config_get_workspace_config_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, DragAndDropConfig)
{
    miracle_drag_and_drop_config_t dnd_config = {
        true,
        mir_input_event_modifier_meta | mir_input_event_modifier_shift
    };
    miracle_config_set_drag_and_drop(
        &wrapper->config,
        &dnd_config);

    auto dnd = miracle_config_get_drag_and_drop(&wrapper->config);
    EXPECT_TRUE(dnd.enabled);
    EXPECT_EQ(dnd.modifiers,
        mir_input_event_modifier_meta | mir_input_event_modifier_shift);
}

TEST_F(CAPIWrapperTest, MouseConfig)
{
    miracle_mouse_config_t mouse_config = {
        mir_pointer_handedness_left,
        0.5,
        2.0,
        3.0,
        mir_pointer_acceleration_adaptive
    };
    miracle_config_set_mouse_config(
        &wrapper->config,
        &mouse_config);

    auto const mouse = miracle_config_get_mouse_config(&wrapper->config);
    EXPECT_EQ(mouse.handedness, mir_pointer_handedness_left);
    EXPECT_EQ(mouse.acceleration_bias, 0.5);

    // This is to fix tests on older systems that are afflicted by
    // https://github.com/canonical/mir/commit/cef548009d8a2933bea8d500e5421e81257317e0
#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(2, 21, 1)
    EXPECT_EQ(mouse.vscroll_speed, 2.0);
    EXPECT_EQ(mouse.hscroll_speed, 3.0);
#endif
    EXPECT_EQ(mouse.acceleration, mir_pointer_acceleration_adaptive);
}

TEST_F(CAPIWrapperTest, TouchpadConfig)
{
    miracle_touchpad_config_t touchpad_config = {
        true, // disable_while_typing
        false, // disable_with_external_mouse
        -0.3f, // acceleration_bias
        1.5f, // vscroll_speed
        0.8f, // hscroll_speed
        false, // tap_to_click
        true, // middle_mouse_button_emulation
        mir_touchpad_click_mode_area_to_click, // click_mode
        mir_touchpad_scroll_mode_edge_scroll // scroll_mode
    };
    miracle_config_set_touchpad_config(
        &wrapper->config,
        &touchpad_config);

    auto const touchpad = miracle_config_get_touchpad_config(&wrapper->config);
    EXPECT_EQ(touchpad.disable_while_typing, true);
    EXPECT_EQ(touchpad.disable_with_external_mouse, false);
    EXPECT_EQ(touchpad.acceleration_bias, -0.3f);
    EXPECT_EQ(touchpad.vscroll_speed, 1.5f);
    EXPECT_EQ(touchpad.hscroll_speed, 0.8f);
    EXPECT_EQ(touchpad.tap_to_click, false);
    EXPECT_EQ(touchpad.middle_mouse_button_emulation, true);
    EXPECT_EQ(touchpad.click_mode, mir_touchpad_click_mode_area_to_click);
    EXPECT_EQ(touchpad.scroll_mode, mir_touchpad_scroll_mode_edge_scroll);
}

TEST_F(CAPIWrapperTest, TouchpadClickModeOptionsCount)
{
    ASSERT_THAT(miracle_config_get_touchpad_click_mode_options_count(), Eq(3));
}

TEST_F(CAPIWrapperTest, TouchpadClickModeOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_touchpad_click_mode_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_touchpad_click_mode_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, TouchpadScrollModeOptionsCount)
{
    ASSERT_THAT(miracle_config_get_touchpad_scroll_mode_options_count(), Eq(4));
}

TEST_F(CAPIWrapperTest, TouchpadScrollModeOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_touchpad_scroll_mode_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_touchpad_scroll_mode_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, CanSetKeymap)
{
    miracle_keymap_t keymap = {
        true, // is_set
        "fr", // language
        true, // has_variant
        "dvorak", // variant
        0 // options_count
    };
    miracle_config_set_keymap(
        &wrapper->config,
        &keymap);

    auto const keymap_result = miracle_config_get_keymap(&wrapper->config);
    EXPECT_TRUE(keymap_result.is_set);
    EXPECT_STREQ(keymap_result.language, "fr");
    EXPECT_TRUE(keymap_result.has_variant);
    EXPECT_STREQ(keymap_result.variant, "dvorak");
}

TEST_F(CAPIWrapperTest, CanSetKeymapOption)
{
    miracle_keymap_t keymap = {
        true, // is_set
        "fr", // language
        true, // has_variant
        "dvorak", // variant
        0 // options_count
    };
    miracle_config_set_keymap(
        &wrapper->config,
        &keymap);

    miracle_config_add_keymap_option(
        &wrapper->config,
        "hi");
    miracle_config_add_keymap_option(
        &wrapper->config,
        "bye");
    auto keymap_result = miracle_config_get_keymap(&wrapper->config);
    EXPECT_EQ(keymap_result.options_count, 2);

    miracle_config_set_keymap_option(
        &wrapper->config,
        0,
        "x");
    auto option = miracle_config_get_keymap_option(
        &wrapper->config,
        0);
    EXPECT_STREQ(option, "x");

    miracle_config_remove_keymap_option(&wrapper->config, 0);
    keymap_result = miracle_config_get_keymap(&wrapper->config);
    EXPECT_EQ(keymap_result.options_count, 1);
    option = miracle_config_get_keymap_option(
        &wrapper->config,
        0);
    EXPECT_STREQ(option, "bye");
}

#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
TEST_F(CAPIWrapperTest, CanSetKeyRepeatRate)
{
    miracle_config_set_key_repeat_rate(&wrapper->config, 5);
    EXPECT_EQ(miracle_config_get_key_repeat_rate(&wrapper->config), 5);
}

TEST_F(CAPIWrapperTest, CanSetKeyRepeatDelay)
{
    miracle_config_set_key_repeat_delay(&wrapper->config, 10);
    EXPECT_EQ(miracle_config_get_key_repeat_delay(&wrapper->config), 10);
}
#endif

TEST_F(CAPIWrapperTest, CanSetHoverClickData)
{
    miracle_hover_click_t hover_click = {
        true,
        123,
        456,
        789
    };
    miracle_config_set_hover_click(
        &wrapper->config,
        &hover_click);
    auto const hover_click_result = miracle_config_get_hover_click(&wrapper->config);
    EXPECT_EQ(hover_click_result.enabled, true);
    EXPECT_EQ(hover_click_result.hover_duration_milliseconds, 123);
    EXPECT_EQ(hover_click_result.cancel_displacement_threshold, 456);
    EXPECT_EQ(hover_click_result.reclick_displacement_threshold, 789);
}

TEST_F(CAPIWrapperTest, CanSetSimulatedSecondaryClickData)
{
    miracle_simulated_secondary_click_t ssc = {
        true,
        123,
        456
    };
    miracle_config_set_simulated_secondary_click(
        &wrapper->config,
        &ssc);
    auto const ssc_result = miracle_config_get_simulated_secondary_click(&wrapper->config);
    EXPECT_EQ(ssc_result.enabled, true);
    EXPECT_EQ(ssc_result.hold_duration_milliseconds, 123);
    EXPECT_EQ(ssc_result.displacement_threshold, 456);
}

TEST_F(CAPIWrapperTest, CanSetOutputFilter)
{
    miracle_output_filter_t output_filter = {
        true,
        "hello"
    };
    miracle_config_set_output_filter(&wrapper->config, &output_filter);
    auto const output_filter_result = miracle_config_get_output_filter(&wrapper->config);
    EXPECT_EQ(output_filter_result.shader_path_enabled, true);
    EXPECT_STREQ(output_filter_result.shader_path, "hello");
}

TEST_F(CAPIWrapperTest, CanSetCursor)
{
    miracle_cursor_t cursor = {
        2.f,
        static_cast<uint>(miracle::CursorFocusMode::Click)
    };
    miracle_config_set_cursor(&wrapper->config, &cursor);
    auto const cursor_result = miracle_config_get_cursor(&wrapper->config);
    EXPECT_EQ(cursor_result.scale, 2.f);
    EXPECT_EQ(cursor_result.focus_mode, static_cast<uint>(miracle::CursorFocusMode::Click));
}

TEST_F(CAPIWrapperTest, CanSetSlowKeys)
{
    miracle_slow_keys_t slow_keys = { true, 500 };
    miracle_config_set_slow_keys(&wrapper->config, &slow_keys);
    auto const slow_keys_result = miracle_config_get_slow_keys(&wrapper->config);
    EXPECT_EQ(slow_keys_result.enabled, true);
    EXPECT_EQ(slow_keys_result.hold_duration_milliseconds, 500);
}

TEST_F(CAPIWrapperTest, CanSetStickyKeys)
{
    miracle_sticky_keys_t sticky_keys = { true, false };
    miracle_config_set_sticky_keys(&wrapper->config, &sticky_keys);
    auto const sticky_keys_result = miracle_config_get_sticky_keys(&wrapper->config);
    EXPECT_EQ(sticky_keys_result.enabled, true);
    EXPECT_EQ(sticky_keys_result.should_disable_if_two_keys_are_pressed_together, false);
}

TEST_F(CAPIWrapperTest, CanSetMagnifier)
{
    miracle_magnifier_t magnifier;
    magnifier.enabled = true;
    magnifier.scale = 2.f;
    magnifier.scale_increment = 0.75f;
    magnifier.width = 800;
    magnifier.height = 600;
    magnifier.size_increment = 200;
    miracle_config_set_magnifier(&wrapper->config, magnifier);
    auto const magnifier_config = miracle_config_get_magnifier(&wrapper->config);
    EXPECT_EQ(magnifier_config.enabled, true);
    EXPECT_EQ(magnifier_config.scale, 2.f);
    EXPECT_EQ(magnifier_config.scale_increment, 0.75f);
    EXPECT_EQ(magnifier.width, 800);
    EXPECT_EQ(magnifier.height, 600);
    EXPECT_EQ(magnifier.size_increment, 200);
}

TEST_F(CAPIWrapperTest, CanSetWorkspaceBackAndForth)
{
    miracle_config_set_workspace_back_and_forth(&wrapper->config, false);
    EXPECT_THAT(miracle_config_get_workspace_back_and_forth(&wrapper->config), Eq(false));
}

TEST_F(CAPIWrapperTest, CanSaveConfigToFile)
{
    // Create a temp file path
    const char* temp_path = "/tmp/miracle_test_config.yaml";

    // Set some config values
    miracle_config_set_primary_modifier(&wrapper->config, mir_input_event_modifier_alt);
    miracle_config_set_inner_gaps_x(&wrapper->config, 20);
    miracle_environment_variable_t env_var = { "TEST", "VALUE" };
    miracle_config_add_environment_variable(&wrapper->config, &env_var);

    // Save the config
    auto save_result = miracle_config_save(temp_path, &wrapper->config);
    EXPECT_TRUE(save_result->success);
    EXPECT_EQ(miracle_save_result_get_error_count(save_result), 0);

    // Clean up
    miracle_save_result_free(save_result);
    std::remove(temp_path);
}

TEST_F(CAPIWrapperTest, SaveFailsForInvalidPath)
{
    // Try to save to invalid path
    const char* invalid_path = "/nonexistent/path/config.yaml";

    auto save_result = miracle_config_save(invalid_path, &wrapper->config);
    EXPECT_FALSE(save_result->success);
    EXPECT_GT(miracle_save_result_get_error_count(save_result), 0);

    // Verify error message contains expected text
    auto error = miracle_save_result_get_error(save_result, 0);
    EXPECT_THAT(std::string(error->message), HasSubstr("Failed to save config"));

    miracle_save_result_free(save_result);
}

TEST_F(CAPIWrapperTest, CanRoundTripConfigThroughSaveAndLoad)
{
    const char* temp_path = "/tmp/miracle_test_roundtrip.yaml";

    // Set some config values
    miracle_config_add_include(&wrapper->config, "/home/hi", 0);
    miracle_config_add_include(&wrapper->config, "/home/bye", 1);
    miracle_config_set_primary_modifier(&wrapper->config, mir_input_event_modifier_alt);
    miracle_config_set_inner_gaps_x(&wrapper->config, 20);

    miracle_environment_variable_t env_var = { "TEST", "VALUE" };
    miracle_config_add_environment_variable(&wrapper->config, &env_var);

    miracle_custom_key_command_t key_command = {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        "test-command"
    };
    miracle_config_add_custom_key_command(
        &wrapper->config,
        &key_command);

    miracle_mouse_config_t mouse_config = {
        mir_pointer_handedness_right,
        0.3,
        0.4,
        0.5,
        mir_pointer_acceleration_adaptive
    };
    miracle_config_set_mouse_config(
        &wrapper->config,
        &mouse_config);

    miracle_touchpad_config_t touchpad_config = {
        true, // disable_while_typing
        true, // disable_with_external_mouse
        -0.25f, // acceleration_bias
        1.3f, // vscroll_speed
        0.9f, // hscroll_speed
        false, // tap_to_click
        true, // middle_mouse_button_emulation
        mir_touchpad_click_mode_finger_count, // click_mode
        mir_touchpad_scroll_mode_two_finger_scroll // scroll_mode
    };
    miracle_config_set_touchpad_config(
        &wrapper->config,
        &touchpad_config);

    miracle_keymap_t keymap = {
        true,
        "fr",
        true,
        "dvorak",
        0
    };
    miracle_config_set_keymap(
        &wrapper->config,
        &keymap);
    miracle_config_add_keymap_option(
        &wrapper->config,
        "hi");
    miracle_config_set_key_repeat_rate(&wrapper->config, 5);
    miracle_config_set_key_repeat_delay(&wrapper->config, 10);

    miracle_hover_click_t hover_click = {
        true,
        123,
        456,
        789
    };
    miracle_config_set_hover_click(
        &wrapper->config,
        &hover_click);

    miracle_simulated_secondary_click_t ssc = {
        true,
        123,
        456
    };
    miracle_config_set_simulated_secondary_click(
        &wrapper->config,
        &ssc);

    miracle_cursor_t cursor = {
        2.f,
        static_cast<uint>(miracle::CursorFocusMode::Click)
    };
    miracle_config_set_cursor(&wrapper->config, &cursor);

    miracle_slow_keys_t slow_keys = { true, 500 };
    miracle_config_set_slow_keys(&wrapper->config, &slow_keys);

    miracle_sticky_keys_t sticky_keys = { true, false };
    miracle_config_set_sticky_keys(&wrapper->config, &sticky_keys);

    miracle_magnifier_t magnifier;
    magnifier.enabled = true;
    magnifier.scale = 2.f;
    magnifier.scale_increment = 0.75f;
    magnifier.width = 800;
    magnifier.height = 600;
    magnifier.size_increment = 200;
    miracle_config_set_magnifier(&wrapper->config, magnifier);
    miracle_config_set_workspace_back_and_forth(&wrapper->config, false);

    // Save the config
    auto save_result = miracle_config_save(temp_path, &wrapper->config);
    EXPECT_TRUE(save_result->success);
    miracle_save_result_free(save_result);

    // Load it back
    auto load_result = miracle_config_load(temp_path);
    EXPECT_EQ(miracle_config_get_error_count(load_result), 0);

    // Verify values match
    auto loaded_config = miracle_config_get_data(load_result);

    EXPECT_THAT(miracle_config_get_num_includes(loaded_config), Eq(2));
    EXPECT_STREQ(miracle_config_get_include(loaded_config, 0), "/home/hi");
    EXPECT_STREQ(miracle_config_get_include(loaded_config, 1), "/home/bye");

    EXPECT_EQ(miracle_config_get_primary_modifier(loaded_config), mir_input_event_modifier_alt);
    EXPECT_EQ(miracle_config_get_inner_gaps_x(loaded_config), 20);

    EXPECT_EQ(miracle_config_get_environment_variable_count(loaded_config), 1);
    auto env = miracle_config_get_environment_variable(loaded_config, 0);
    EXPECT_STREQ(env.key, "TEST");
    EXPECT_STREQ(env.value, "VALUE");

    EXPECT_EQ(miracle_config_get_custom_key_command_count(loaded_config), 1);
    auto cmd = miracle_config_get_custom_key_command(loaded_config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_down);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_meta);
    EXPECT_EQ(cmd.key, KEY_F);
    EXPECT_STREQ(cmd.command, "test-command");

    auto const mouse = miracle_config_get_mouse_config(loaded_config);
    EXPECT_EQ(mouse.handedness, mir_pointer_handedness_right);
    EXPECT_EQ(mouse.acceleration_bias, 0.3);
    // This is to fix tests on older systems that are afflicted by
    // https://github.com/canonical/mir/commit/cef548009d8a2933bea8d500e5421e81257317e0
#if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(2, 21, 1)
    EXPECT_EQ(mouse.vscroll_speed, 0.4);
    EXPECT_EQ(mouse.hscroll_speed, 0.5);
#endif
    EXPECT_EQ(mouse.acceleration, mir_pointer_acceleration_adaptive);

    auto const touchpad = miracle_config_get_touchpad_config(loaded_config);
    EXPECT_EQ(touchpad.disable_while_typing, true);
    EXPECT_EQ(touchpad.disable_with_external_mouse, true);
    EXPECT_EQ(touchpad.acceleration_bias, -0.25f);
    EXPECT_EQ(touchpad.vscroll_speed, 1.3f);
    EXPECT_EQ(touchpad.hscroll_speed, 0.9f);
    EXPECT_EQ(touchpad.tap_to_click, false);
    EXPECT_EQ(touchpad.middle_mouse_button_emulation, true);
    EXPECT_EQ(touchpad.click_mode, mir_touchpad_click_mode_finger_count);
    EXPECT_EQ(touchpad.scroll_mode, mir_touchpad_scroll_mode_two_finger_scroll);

#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
    auto const keymap_result = miracle_config_get_keymap(loaded_config);
    EXPECT_TRUE(keymap_result.is_set);
    EXPECT_STREQ(keymap_result.language, "fr");
    EXPECT_TRUE(keymap_result.has_variant);
    EXPECT_STREQ(keymap_result.variant, "dvorak");
    EXPECT_EQ(keymap_result.options_count, 1);
    EXPECT_STREQ(miracle_config_get_keymap_option(loaded_config, 0), "hi");
    EXPECT_EQ(miracle_config_get_key_repeat_rate(loaded_config), 5);
    EXPECT_EQ(miracle_config_get_key_repeat_delay(loaded_config), 10);
#endif

    auto const hover_click_result = miracle_config_get_hover_click(loaded_config);
    EXPECT_EQ(hover_click_result.enabled, true);
    EXPECT_EQ(hover_click_result.hover_duration_milliseconds, 123);
    EXPECT_EQ(hover_click_result.cancel_displacement_threshold, 456);
    EXPECT_EQ(hover_click_result.reclick_displacement_threshold, 789);

    auto const ssc_result = miracle_config_get_simulated_secondary_click(loaded_config);
    EXPECT_EQ(ssc_result.enabled, true);
    EXPECT_EQ(ssc_result.hold_duration_milliseconds, 123);
    EXPECT_EQ(ssc_result.displacement_threshold, 456);

    auto const cursor_result = miracle_config_get_cursor(loaded_config);
    EXPECT_EQ(cursor_result.scale, 2.f);
    EXPECT_EQ(cursor_result.focus_mode, static_cast<uint>(miracle::CursorFocusMode::Click));

    auto const slow_keys_result = miracle_config_get_slow_keys(loaded_config);
    EXPECT_EQ(slow_keys_result.enabled, true);
    EXPECT_EQ(slow_keys_result.hold_duration_milliseconds, 500);

    auto const sticky_keys_result = miracle_config_get_sticky_keys(loaded_config);
    EXPECT_EQ(sticky_keys_result.enabled, true);
    EXPECT_EQ(sticky_keys_result.should_disable_if_two_keys_are_pressed_together, false);

    auto const magnifier_config = miracle_config_get_magnifier(loaded_config);
    EXPECT_EQ(magnifier_config.enabled, true);
    EXPECT_EQ(magnifier_config.scale, 2.f);
    EXPECT_EQ(magnifier_config.scale_increment, 0.75f);
    EXPECT_EQ(magnifier_config.width, 800);
    EXPECT_EQ(magnifier_config.height, 600);
    EXPECT_EQ(magnifier_config.size_increment, 200);

    EXPECT_EQ(miracle_config_get_workspace_back_and_forth(&wrapper->config), false);

    // Clean up
    miracle_config_free(load_result);
    std::remove(temp_path);
}

TEST_F(CAPIWrapperTest, CanAddPlugins)
{
    miracle_plugin_t plugin = { "/usr/lib/miracle/plugins/libsample.wasm", "sample" };
    miracle_config_add_plugin(&wrapper->config, &plugin);
    EXPECT_EQ(miracle_config_get_plugin_count(&wrapper->config), 1);
    auto got = miracle_config_get_plugin(&wrapper->config, 0);
    EXPECT_STREQ(got.path, "/usr/lib/miracle/plugins/libsample.wasm");
    EXPECT_STREQ(got.name, "sample");
}

TEST_F(CAPIWrapperTest, CanSetPlugins)
{
    miracle_plugin_t plugin = { "/usr/lib/miracle/plugins/libsample.wasm", "sample" };
    miracle_config_add_plugin(&wrapper->config, &plugin);

    plugin = { "/opt/plugins/libother.wasm", "other" };
    miracle_config_set_plugin(&wrapper->config, 0, &plugin);

    auto got = miracle_config_get_plugin(&wrapper->config, 0);
    EXPECT_STREQ(got.path, "/opt/plugins/libother.wasm");
    EXPECT_STREQ(got.name, "other");
}

TEST_F(CAPIWrapperTest, CanRemovePlugins)
{
    miracle_plugin_t plugin = { "/usr/lib/miracle/plugins/libsample.wasm", "sample" };
    miracle_config_add_plugin(&wrapper->config, &plugin);
    EXPECT_TRUE(miracle_config_remove_plugin(&wrapper->config, 0));
    EXPECT_EQ(miracle_config_get_plugin_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, PluginsRoundTrip)
{
    const char* temp_path = "/tmp/miracle_test_plugins.yaml";
    miracle_plugin_t plugin = { "/usr/lib/miracle/plugins/libsample.wasm", "sample" };
    miracle_config_add_plugin(&wrapper->config, &plugin);
    plugin = { "/opt/plugins/libother.wasm", "other" };
    miracle_config_add_plugin(&wrapper->config, &plugin);

    auto save_result = miracle_config_save(temp_path, &wrapper->config);
    ASSERT_TRUE(save_result->success);
    miracle_save_result_free(save_result);

    auto load_result = miracle_config_load(temp_path);
    ASSERT_EQ(miracle_config_get_error_count(load_result), 0);
    auto loaded = miracle_config_get_data(load_result);
    ASSERT_EQ(miracle_config_get_plugin_count(loaded), 2);

    miracle_config_free(load_result);
    std::remove(temp_path);
}
