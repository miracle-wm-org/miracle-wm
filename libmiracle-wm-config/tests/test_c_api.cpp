#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <miracle/miracle-wm-config-c.h>
#include <miracle/miracle-wm-config.h>

using namespace testing;

class CAPIWrapperTest : public Test {
protected:
    void SetUp() override {
        config = std::make_unique<miracle::ConfigData>();
        wrapper = std::make_unique<miracle_config_load_result_t>();
        wrapper->config._internal = config.get();
    }

    std::unique_ptr<miracle::ConfigData> config;
    std::unique_ptr<miracle_config_load_result_t> wrapper;
};

TEST_F(CAPIWrapperTest, PrimaryModifier) {
    uint modifier = mir_input_event_modifier_alt;
    miracle_config_set_primary_modifier(&wrapper->config, modifier);
    EXPECT_EQ(miracle_config_get_primary_modifier(&wrapper->config), modifier);
}

TEST_F(CAPIWrapperTest, CustomKeyCommands) {
    // Test add/get
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

    // Test remove
    EXPECT_TRUE(miracle_config_remove_custom_key_command(&wrapper->config, 0));
    EXPECT_EQ(miracle_config_get_custom_key_command_count(&wrapper->config), 0);
}

TEST_F(CAPIWrapperTest, StartupApps) {
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

TEST_F(CAPIWrapperTest, BorderConfig) {
    float focus_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    
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

TEST_F(CAPIWrapperTest, AnimationDefinitions) {
    miracle_animation_definition_t def = {
        MIRACLE_ANIMATION_TYPE_SLIDE,
        MIRACLE_EASE_FUNCTION_EASE_OUT_BOUNCE,
        0.5f,
        1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f
    };
    
    miracle_config_set_animation_definition(
        &wrapper->config,
        MIRACLE_ANIMATABLE_EVENT_WINDOW_OPEN,
        &def);
    
    auto result = miracle_config_get_animation_definition(
        &wrapper->config,
        MIRACLE_ANIMATABLE_EVENT_WINDOW_OPEN);
    
    EXPECT_EQ(result.type, MIRACLE_ANIMATION_TYPE_SLIDE);
    EXPECT_EQ(result.function, MIRACLE_EASE_FUNCTION_EASE_OUT_BOUNCE);
    EXPECT_FLOAT_EQ(result.duration_seconds, 0.5f);
}

TEST_F(CAPIWrapperTest, WorkspaceConfigs) {
    miracle_config_add_workspace_config(
        &wrapper->config,
        1,
        static_cast<int>(miracle::ContainerType::horizontal),
        "Main");
    
    EXPECT_EQ(miracle_config_get_workspace_config_count(&wrapper->config), 1);
    
    auto ws = miracle_config_get_workspace_config(&wrapper->config, 0);
    EXPECT_EQ(ws.num, 1);
    EXPECT_EQ(ws.container_type, static_cast<int>(miracle::ContainerType::horizontal));
    EXPECT_STREQ(ws.name, "Main");
}

TEST_F(CAPIWrapperTest, DragAndDropConfig) {
    miracle_config_set_drag_and_drop(
        &wrapper->config,
        true,
        mir_input_event_modifier_meta | mir_input_event_modifier_shift);
    
    auto dnd = miracle_config_get_drag_and_drop(&wrapper->config);
    EXPECT_TRUE(dnd.enabled);
    EXPECT_EQ(dnd.modifiers, 
        mir_input_event_modifier_meta | mir_input_event_modifier_shift);
}
