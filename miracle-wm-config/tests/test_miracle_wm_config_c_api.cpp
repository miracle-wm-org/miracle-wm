#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input-event-codes.h>
#include <miracle/miracle-wm-config-c.h>
#include <miracle/miracle-wm-config.h>

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

TEST_F(CAPIWrapperTest, AnimateableEventOptionsCount)
{
    ASSERT_THAT(miracle_config_get_animateable_event_options_count(), Eq(static_cast<uint>(miracle::AnimateableEvent::max)));
}

TEST_F(CAPIWrapperTest, AnimateableEventOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_animateable_event_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_animateable_event_option(i).name, Ne(nullptr));
    }
}

TEST_F(CAPIWrapperTest, AnimationTypeOptionsCount)
{
    ASSERT_THAT(miracle_config_get_animation_type_options_count(), Eq(static_cast<uint>(miracle::AnimationType::max)));
}

TEST_F(CAPIWrapperTest, AnimationTypeOptionsCanBeFound)
{
    for (uint i = 0; i < miracle_config_get_animation_type_options_count(); i++)
    {
        ASSERT_THAT(miracle_config_get_animation_type_option(i).name, Ne(nullptr));
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
    ASSERT_THAT(miracle_config_get_layout_options_count(), Eq(static_cast<uint>(miracle::ContainerType::max)));
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
    miracle_config_add_custom_key_command(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command");

    EXPECT_EQ(miracle_config_get_custom_key_command_count(&wrapper->config), 1);

    auto cmd = miracle_config_get_custom_key_command(&wrapper->config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_down);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_meta);
    EXPECT_EQ(cmd.key, 10);
    EXPECT_STREQ(cmd.command, "test-command");
}

TEST_F(CAPIWrapperTest, CanEditCustomKeyCommand)
{
    miracle_config_add_custom_key_command(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command");

    miracle_config_edit_custom_key_command(
        &wrapper->config,
        0,
        mir_keyboard_action_up,
        mir_input_event_modifier_alt,
        12,
        "test-command-2");

    auto cmd = miracle_config_get_custom_key_command(&wrapper->config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_up);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_alt);
    EXPECT_EQ(cmd.key, 12);
    EXPECT_STREQ(cmd.command, "test-command-2");
}

TEST_F(CAPIWrapperTest, CannotEditCustomKeyCommandWithIndexLessThanZero)
{
    miracle_config_add_custom_key_command(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command");

    miracle_config_edit_custom_key_command(
        &wrapper->config,
        -1,
        mir_keyboard_action_up,
        mir_input_event_modifier_alt,
        12,
        "test-command-2");

    auto cmd = miracle_config_get_custom_key_command(&wrapper->config, 0);
    EXPECT_EQ(cmd.action, mir_keyboard_action_down);
    EXPECT_EQ(cmd.modifiers, mir_input_event_modifier_meta);
    EXPECT_EQ(cmd.key, 10);
    EXPECT_STREQ(cmd.command, "test-command");
}

TEST_F(CAPIWrapperTest, CanRemoveCustomKeyCommand)
{
    // Test add/get
    miracle_config_add_custom_key_command(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command");

    EXPECT_TRUE(miracle_config_remove_custom_key_command(&wrapper->config, 0));
    EXPECT_EQ(miracle_config_get_custom_key_command_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, CanClearCustomKeyCommand)
{
    // Test add/get
    miracle_config_add_custom_key_command(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command");

    miracle_config_clear_custom_key_commands(&wrapper->config);
    EXPECT_EQ(miracle_config_get_custom_key_command_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, CanAddBuiltInKeyCommand)
{
    miracle_config_add_built_in_key_command_override(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen));
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
    miracle_config_add_built_in_key_command_override(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen));

    miracle_config_set_built_in_key_command_override(
        &wrapper->config,
        0,
        mir_keyboard_action_up,
        mir_input_event_modifier_alt,
        KEY_K,
        static_cast<uint>(miracle::DefaultKeyCommand::MoveDown));

    auto const key_command = miracle_config_get_built_in_key_command_override(
        &wrapper->config, 0);
    EXPECT_THAT(key_command.action, Eq(mir_keyboard_action_up));
    EXPECT_THAT(key_command.modifiers, Eq(mir_input_event_modifier_alt));
    EXPECT_THAT(key_command.key, Eq(KEY_K));
    EXPECT_THAT(key_command.command, Eq(static_cast<uint>(miracle::DefaultKeyCommand::MoveDown)));
}

TEST_F(CAPIWrapperTest, CanRemoveBuiltInKeyCommand)
{
    miracle_config_add_built_in_key_command_override(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        static_cast<uint>(miracle::DefaultKeyCommand::Fullscreen));

    EXPECT_THAT(miracle_config_remove_built_in_key_command_override(&wrapper->config, 0), Eq(true));
}

TEST_F(CAPIWrapperTest, CanAddStartupApps)
{
    miracle_config_add_startup_app(
        &wrapper->config,
        "test-app",
        true,
        false,
        true,
        false);

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
    miracle_config_add_startup_app(
        &wrapper->config,
        "test-app",
        true,
        false,
        true,
        false);

    miracle_config_set_startup_app(
        &wrapper->config,
        0,
        "test-app-2",
        false,
        true,
        false,
        true);

    auto app = miracle_config_get_startup_app(&wrapper->config, 0);
    EXPECT_STREQ(app.command, "test-app-2");
    EXPECT_FALSE(app.restart_on_death);
    EXPECT_TRUE(app.no_startup_id);
    EXPECT_FALSE(app.should_halt_compositor_on_death);
    EXPECT_TRUE(app.in_systemd_scope);
}

TEST_F(CAPIWrapperTest, CanRemoveStartupApps)
{
    miracle_config_add_startup_app(
        &wrapper->config,
        "test-app",
        true,
        false,
        true,
        false);

    miracle_config_remove_startup_app(&wrapper->config, 0);
    EXPECT_EQ(miracle_config_get_startup_app_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, CanAddEnvironmentVariable)
{
    miracle_config_add_environment_variable(
        &wrapper->config,
        "first",
        "second");

    EXPECT_THAT(miracle_config_get_environment_variable_count(&wrapper->config), Eq(1));
    auto const env_variable = miracle_config_get_environment_variable(&wrapper->config, 0);
    EXPECT_THAT(std::string(env_variable.key), Eq("first"));
    EXPECT_THAT(std::string(env_variable.value), Eq("second"));
}

TEST_F(CAPIWrapperTest, CanSetEnvironmentVariable)
{
    miracle_config_add_environment_variable(
        &wrapper->config,
        "first",
        "second");

    miracle_config_set_environment_variable(
        &wrapper->config,
        0,
        "third",
        "fourth");

    auto const env_variable = miracle_config_get_environment_variable(&wrapper->config, 0);
    EXPECT_THAT(std::string(env_variable.key), Eq("third"));
    EXPECT_THAT(std::string(env_variable.value), Eq("fourth"));
}

TEST_F(CAPIWrapperTest, CanRemoveEnvironmentVariable)
{
    miracle_config_add_environment_variable(
        &wrapper->config,
        "first",
        "second");
    miracle_config_remove_environment_variable(&wrapper->config, 0);

    EXPECT_THAT(miracle_config_get_environment_variable_count(&wrapper->config), Eq(0));
}

TEST_F(CAPIWrapperTest, BorderConfig)
{
    float focus_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    float color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

    miracle_config_set_border_config(
        &wrapper->config,
        2,
        focus_color,
        color);

    auto border = miracle_config_get_border_config(&wrapper->config);
    EXPECT_EQ(border.size, 2);
    EXPECT_THAT(border.focus_color, ElementsAre(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_THAT(border.color, ElementsAre(0.5f, 0.5f, 0.5f, 1.0f));
}

TEST_F(CAPIWrapperTest, CanSetAnimationDefinitions)
{
    miracle_animation_definition_t const def = {
        false,
        static_cast<uint>(miracle::AnimationType::slide),
        static_cast<uint>(miracle::EaseFunction::ease_out_bounce),
        0.5f,
        1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f
    };

    miracle_config_set_animation_definition(
        &wrapper->config,
        static_cast<uint>(miracle::AnimateableEvent::window_open),
        &def);

    auto result = miracle_config_get_animation_definition(
        &wrapper->config,
        static_cast<uint>(miracle::AnimateableEvent::window_open));

    EXPECT_EQ(result.type, static_cast<uint>(miracle::AnimationType::slide));
    EXPECT_EQ(result.function, static_cast<uint>(miracle::EaseFunction::ease_out_bounce));
    EXPECT_FLOAT_EQ(result.duration_seconds, 0.5f);
}

TEST_F(CAPIWrapperTest, CanResetAnimationDefinitions)
{
    miracle_animation_definition_t const def = {
        false,
        static_cast<uint>(miracle::AnimationType::slide),
        static_cast<uint>(miracle::EaseFunction::ease_out_bounce),
        0.5f,
        1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f
    };

    miracle_config_set_animation_definition(
        &wrapper->config,
        static_cast<uint>(miracle::AnimateableEvent::window_open),
        &def);

    miracle_config_reset_animation_definition(
        &wrapper->config,
        static_cast<uint>(miracle::AnimateableEvent::window_open));

    auto result = miracle_config_get_animation_definition(
        &wrapper->config,
        static_cast<uint>(miracle::AnimateableEvent::window_open));

    EXPECT_EQ(result.type, static_cast<uint>(miracle::AnimationType::fade_in));
    EXPECT_EQ(result.function, static_cast<uint>(miracle::EaseFunction::linear));
    EXPECT_FLOAT_EQ(result.duration_seconds, 0.3f);
}

TEST_F(CAPIWrapperTest, CanAddWorkspaceConfig)
{
    miracle_config_add_workspace_config(
        &wrapper->config,
        1,
        static_cast<int>(miracle::ContainerType::leaf),
        "Main");

    EXPECT_EQ(miracle_config_get_workspace_config_count(&wrapper->config), 1);

    auto ws = miracle_config_get_workspace_config(&wrapper->config, 0);
    EXPECT_EQ(ws.num, 1);
    EXPECT_EQ(ws.container_type, static_cast<int>(miracle::ContainerType::leaf));
    EXPECT_STREQ(ws.name, "Main");
}

TEST_F(CAPIWrapperTest, CanSetWorkspaceConfig)
{
    miracle_config_add_workspace_config(
        &wrapper->config,
        1,
        static_cast<int>(miracle::ContainerType::leaf),
        "Main");

    miracle_config_set_workspace_config(
        &wrapper->config,
        0,
        2,
        static_cast<int>(miracle::ContainerType::stack),
        "Other");

    auto ws = miracle_config_get_workspace_config(&wrapper->config, 0);
    EXPECT_EQ(ws.num, 2);
    EXPECT_EQ(ws.container_type, static_cast<int>(miracle::ContainerType::stack));
    EXPECT_STREQ(ws.name, "Other");
}

TEST_F(CAPIWrapperTest, CanRemoveWorkspaceConfig)
{
    miracle_config_add_workspace_config(
        &wrapper->config,
        1,
        static_cast<int>(miracle::ContainerType::leaf),
        "Main");

    miracle_config_remove_workspace_config(&wrapper->config, 0);
    EXPECT_EQ(miracle_config_get_workspace_config_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, DragAndDropConfig)
{
    miracle_config_set_drag_and_drop(
        &wrapper->config,
        true,
        mir_input_event_modifier_meta | mir_input_event_modifier_shift);

    auto dnd = miracle_config_get_drag_and_drop(&wrapper->config);
    EXPECT_TRUE(dnd.enabled);
    EXPECT_EQ(dnd.modifiers,
        mir_input_event_modifier_meta | mir_input_event_modifier_shift);
}

TEST_F(CAPIWrapperTest, CanSaveConfigToFile)
{
    // Create a temp file path
    const char* temp_path = "/tmp/miracle_test_config.yaml";

    // Set some config values
    miracle_config_set_primary_modifier(&wrapper->config, mir_input_event_modifier_alt);
    miracle_config_set_inner_gaps_x(&wrapper->config, 20);
    miracle_config_add_environment_variable(&wrapper->config, "TEST", "VALUE");

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
    miracle_config_set_primary_modifier(&wrapper->config, mir_input_event_modifier_alt);
    miracle_config_set_inner_gaps_x(&wrapper->config, 20);
    miracle_config_add_environment_variable(&wrapper->config, "TEST", "VALUE");
    miracle_config_add_custom_key_command(
        &wrapper->config,
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        KEY_F,
        "test-command");

    // Save the config
    auto save_result = miracle_config_save(temp_path, &wrapper->config);
    EXPECT_TRUE(save_result->success);
    miracle_save_result_free(save_result);

    // Load it back
    auto load_result = miracle_config_load(temp_path);
    EXPECT_EQ(miracle_config_get_error_count(load_result), 0);

    // Verify values match
    auto loaded_config = miracle_config_get_data(load_result);
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

    // Clean up
    miracle_config_free(load_result);
    std::remove(temp_path);
}
