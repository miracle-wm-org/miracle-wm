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

#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <miracle/cpp/config-cpp.h>
#include <vector>

using namespace testing;

class KeymapConfigurationTest : public Test
{
};

TEST_F(KeymapConfigurationTest, KeymapToString)
{
    miracle::KeymapConfiguration config;
    config.language = "en";
    config.variant = "qwerty";
    config.options.emplace_back("a");
    config.options.emplace_back("b");
    EXPECT_EQ(config.to_string(), "en+qwerty+a,b");
}

TEST_F(KeymapConfigurationTest, CanMergeMiracleConfig)
{
    miracle::ConfigData first;
    first.primary_modifier = mir_input_event_modifier_alt;
    first.primary_button = mir_pointer_button_secondary;
    first.custom_key_commands->push_back(miracle::CustomKeyCommand {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        10,
        "test-command" });
    first.inner_gaps = miracle::Gaps(0, 0, 0, 0);
    first.outer_gaps = miracle::Gaps(0, 0, 0, 0);
    first.startup_apps->push_back(miracle::StartupApp {
        "first" });
    first.terminal = "terminal";
    first.resize_jump = 10;
    first.environment_variables->push_back(miracle::EnvironmentVariable {
        "first", "first" });
    first.border_config = miracle::BorderConfig {
        .size = 10
    };
    first.animations_enabled = false;
    std::vector<miracle::BuiltInAnimationDefinition> first_animations;
    first_animations.push_back(
        miracle::BuiltInAnimationDefinition { .type = miracle::BultInAnimationType::fade });
    first.animation_definitions.value[0] = miracle::AnimationDefinition {
        .duration_seconds = 5,
        .data = first_animations
    };
    first.workspace_configs->push_back(miracle::WorkspaceConfig {
        .num = 2,
        .name = "second",
    });
    first.move_modifier = mir_input_event_modifier_shift;
    first.drag_and_drop = miracle::DragAndDropConfiguration {
        .enabled = false,
    };
    first.mouse_configuration = miral::InputConfiguration::Mouse();
    first.mouse_configuration->acceleration_bias(2.f);
    first.keyboard_configuration = miral::InputConfiguration::Keyboard();
    first.keyboard_configuration->set_repeat_delay(6.f);
    first.keymap = miracle::KeymapConfiguration {
        .language = "en",
        .variant = std::nullopt,
        .options = std::vector<std::string> {}
    };
    first.simulated_secondary_click = miracle::SimulatedSecondaryClickConfiguration {
        .enabled = false
    };
    first.output_filter = miracle::OutputFilterConfiguration {
        .shader_path = "/home"
    };
    first.cursor = miracle::CursorConfiguration {
        .scale = 2.f
    };
    first.slow_keys = miracle::SlowKeysConfiguration {
        .enabled = false
    };
    first.sticky_keys = miracle::StickyKeysConfiguration {
        .enabled = false
    };
    first.magnifier = miracle::MagnifierConfiguration {
        .enabled = false
    };
    first.touchpad = miracle::TouchpadConfiguration {
        .disable_while_typing = true,
        .acceleration_bias = 1.0f,
        .vscroll_speed = 1.1f,
        .hscroll_speed = 1.2f,
        .tap_to_click = false,
        .middle_mouse_button_emulation = true
    };

    miracle::ConfigData second;
    second.primary_modifier = mir_input_event_modifier_shift;
    second.primary_button = mir_pointer_button_primary;
    second.custom_key_commands->push_back(miracle::CustomKeyCommand {
        mir_keyboard_action_down,
        mir_input_event_modifier_meta,
        12,
        "second-test-command" });
    second.inner_gaps = miracle::Gaps(1, 2, 3, 4);
    second.outer_gaps = miracle::Gaps(5, 6, 7, 8);
    second.startup_apps->push_back(miracle::StartupApp {
        "second" });
    second.terminal = "second-terminal";
    second.resize_jump = 20;
    second.environment_variables->push_back(miracle::EnvironmentVariable {
        "second", "second" });
    second.border_config = miracle::BorderConfig {
        .size = 8
    };
    second.animations_enabled = true;
    std::vector<miracle::BuiltInAnimationDefinition> second_animations;
    second_animations.push_back(
        miracle::BuiltInAnimationDefinition { .type = miracle::BultInAnimationType::grow });
    second.animation_definitions.value[0] = miracle::AnimationDefinition {
        .duration_seconds = 8,
        .data = second_animations
    };
    second.workspace_configs->push_back(miracle::WorkspaceConfig {
        .num = 2,
        .name = "second",
    });
    second.move_modifier = mir_input_event_modifier_shift;
    second.drag_and_drop = miracle::DragAndDropConfiguration {
        .enabled = true
    };
    second.mouse_configuration = miral::InputConfiguration::Mouse();
    second.mouse_configuration->acceleration_bias(4.f);
    second.keyboard_configuration = miral::InputConfiguration::Keyboard();
    second.keyboard_configuration->set_repeat_delay(4.f);
    second.keymap = miracle::KeymapConfiguration {
        .language = "fr",
        .variant = std::nullopt,
        .options = std::vector<std::string> {}
    };
    second.simulated_secondary_click = miracle::SimulatedSecondaryClickConfiguration {
        .enabled = true
    };
    second.output_filter = miracle::OutputFilterConfiguration {
        .shader_path = "/outside"
    };
    second.cursor = miracle::CursorConfiguration {
        .scale = 4.f
    };
    second.slow_keys = miracle::SlowKeysConfiguration {
        .enabled = true
    };
    second.sticky_keys = miracle::StickyKeysConfiguration {
        .enabled = true
    };
    second.magnifier = miracle::MagnifierConfiguration {
        .enabled = true
    };
    second.touchpad = miracle::TouchpadConfiguration {
        .disable_while_typing = false,
        .acceleration_bias = 3.0f,
        .vscroll_speed = 2.1f,
        .hscroll_speed = 2.2f,
        .tap_to_click = true,
        .middle_mouse_button_emulation = false
    };

    auto const merged = first.merge_with(second);
    EXPECT_THAT(*merged.primary_modifier, Eq(mir_input_event_modifier_shift));
    EXPECT_THAT(*merged.primary_button, Eq(mir_pointer_button_primary));
    EXPECT_THAT(merged.custom_key_commands->size(), Eq(2));
    EXPECT_THAT(*merged.inner_gaps, Eq(miracle::Gaps(1, 2, 3, 4)));
    EXPECT_THAT(*merged.outer_gaps, Eq(miracle::Gaps(5, 6, 7, 8)));
    EXPECT_THAT(merged.startup_apps->size(), Eq(2));
    EXPECT_THAT(merged.startup_apps.value[0].command, Eq("second"));
    EXPECT_THAT(merged.startup_apps.value[1].command, Eq("first"));
    EXPECT_THAT(*merged.terminal, Eq("second-terminal"));
    EXPECT_THAT(*merged.resize_jump, Eq(20));
    EXPECT_THAT(merged.environment_variables->size(), Eq(2));
    EXPECT_THAT(merged.environment_variables.value[0].key, Eq("second"));
    EXPECT_THAT(merged.environment_variables.value[0].value, Eq("second"));
    EXPECT_THAT(merged.environment_variables.value[1].key, Eq("first"));
    EXPECT_THAT(merged.environment_variables.value[1].value, Eq("first"));
    EXPECT_THAT(merged.border_config->size, Eq(8));
    EXPECT_THAT(*merged.animations_enabled, Eq(true));
    EXPECT_THAT(merged.animation_definitions.value[0].data[0].type, Eq(miracle::BultInAnimationType::fade));
    EXPECT_THAT(merged.animation_definitions.value[0].duration_seconds, Eq(5));
    EXPECT_THAT(merged.workspace_configs->size(), Eq(2));
    EXPECT_THAT(merged.workspace_configs.value[0].num, Eq(2));
    EXPECT_THAT(merged.workspace_configs.value[0].name, Eq("second"));
    EXPECT_THAT(merged.workspace_configs.value[1].num, Eq(2));
    EXPECT_THAT(merged.workspace_configs.value[1].name, Eq("second"));
    EXPECT_THAT(*merged.move_modifier, Eq(mir_input_event_modifier_shift));
    EXPECT_THAT(merged.drag_and_drop->enabled, Eq(true));
    // TODO: This seems to not work right now, probably a Mir issue
    //  EXPECT_THAT(*merged.mouse_configuration->acceleration_bias(), Eq(4.f));
    EXPECT_THAT(*merged.keyboard_configuration->repeat_delay(), Eq(4.f));
    EXPECT_THAT(merged.keymap->value().language, Eq("fr"));
    EXPECT_THAT(merged.simulated_secondary_click->enabled, Eq(true));
    EXPECT_THAT(merged.output_filter->shader_path, Eq("/outside"));
    EXPECT_THAT(merged.cursor->scale, Eq(4.f));
    EXPECT_THAT(merged.slow_keys->enabled, Eq(true));
    EXPECT_THAT(merged.sticky_keys->enabled, Eq(true));
    EXPECT_THAT(merged.magnifier->enabled, Eq(true));
    EXPECT_THAT(merged.touchpad->disable_while_typing, Eq(false));
    EXPECT_THAT(merged.touchpad->acceleration_bias, Eq(3.0f));
    EXPECT_THAT(merged.touchpad->vscroll_speed, Eq(2.1f));
    EXPECT_THAT(merged.touchpad->hscroll_speed, Eq(2.2f));
    EXPECT_THAT(merged.touchpad->tap_to_click, Eq(true));
    EXPECT_THAT(merged.touchpad->middle_mouse_button_emulation, Eq(false));
}

TEST_F(KeymapConfigurationTest, KeymapLanguageOnly)
{
    miracle::KeymapConfiguration config;
    config.language = "en";
    EXPECT_EQ(config.to_string(), "en+ ");
}

TEST_F(KeymapConfigurationTest, KeymapLanguageAndOptions)
{
    miracle::KeymapConfiguration config;
    config.language = "en";
    config.options.emplace_back("a");
    EXPECT_EQ(config.to_string(), "en+ +a");
}

TEST_F(KeymapConfigurationTest, KeymapVariant)
{
    miracle::KeymapConfiguration config;
    config.language = "en";
    config.variant = "qwerty";
    EXPECT_EQ(config.to_string(), "en+qwerty");
}

class ReadCursorThemeFromFileTest : public Test
{
protected:
    std::filesystem::path tmp_path;

    void SetUp() override
    {
        tmp_path = std::filesystem::temp_directory_path() / "miracle_test_cursor_theme.yaml";
    }

    void TearDown() override
    {
        std::filesystem::remove(tmp_path);
    }

    void write_yaml(std::string const& content)
    {
        std::ofstream f(tmp_path);
        f << content;
    }
};

TEST_F(ReadCursorThemeFromFileTest, ReturnsNulloptForNonExistentFile)
{
    auto result = miracle::read_cursor_theme_from_file("/nonexistent/path/config.yaml");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ReadCursorThemeFromFileTest, ReturnsNulloptWhenNoCursorSection)
{
    write_yaml("inner_gaps:\n  top: 10\n");
    auto result = miracle::read_cursor_theme_from_file(tmp_path.string());
    EXPECT_FALSE(result.has_value());
}

TEST_F(ReadCursorThemeFromFileTest, ReturnsNulloptWhenNoThemeKey)
{
    write_yaml("cursor:\n  scale: 1.5\n");
    auto result = miracle::read_cursor_theme_from_file(tmp_path.string());
    EXPECT_FALSE(result.has_value());
}

TEST_F(ReadCursorThemeFromFileTest, ReturnsNulloptForEmptyTheme)
{
    write_yaml("cursor:\n  theme: \"\"\n");
    auto result = miracle::read_cursor_theme_from_file(tmp_path.string());
    EXPECT_FALSE(result.has_value());
}

TEST_F(ReadCursorThemeFromFileTest, ReturnsThemeWhenSet)
{
    write_yaml("cursor:\n  theme: Adwaita\n");
    auto result = miracle::read_cursor_theme_from_file(tmp_path.string());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Adwaita");
}

TEST_F(ReadCursorThemeFromFileTest, ReturnsColonSeparatedTheme)
{
    write_yaml("cursor:\n  theme: \"Adwaita:DMZ-White\"\n");
    auto result = miracle::read_cursor_theme_from_file(tmp_path.string());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Adwaita:DMZ-White");
}

TEST_F(ReadCursorThemeFromFileTest, ReturnsNulloptForInvalidYaml)
{
    write_yaml(": invalid: yaml: [\n");
    auto result = miracle::read_cursor_theme_from_file(tmp_path.string());
    EXPECT_FALSE(result.has_value());
}

class AnimationConfigurationTest : public Test
{
protected:
    std::filesystem::path tmp_path;

    void SetUp() override
    {
        tmp_path = std::filesystem::temp_directory_path() / "miracle_test_animations.yaml";
    }

    void TearDown() override
    {
        std::filesystem::remove(tmp_path);
    }

    void write_yaml(std::string const& content)
    {
        std::ofstream f(tmp_path);
        f << content;
    }

    static miracle::AnimationDefinition default_for(miracle::AnimateableEvent event)
    {
        return miracle::ConfigData::get_default_animation_definition(event);
    }
};

TEST_F(AnimationConfigurationTest, WindowOpenDefaultGrowsAndFadesIn)
{
    auto const def = default_for(miracle::AnimateableEvent::window_open);
    EXPECT_TRUE(def.is_default);
    EXPECT_THAT(def.duration_seconds, FloatEq(0.2f));
    ASSERT_THAT(def.data.size(), Eq(2u));

    EXPECT_THAT(def.data[0].type, Eq(miracle::BultInAnimationType::grow));
    EXPECT_THAT(def.data[0].function, Eq(miracle::EaseFunction::ease_out_quart));
    EXPECT_THAT(def.data[0].scale, FloatEq(0.9f));

    EXPECT_THAT(def.data[1].type, Eq(miracle::BultInAnimationType::fade));
    EXPECT_THAT(def.data[1].function, Eq(miracle::EaseFunction::ease_out_quad));
}

TEST_F(AnimationConfigurationTest, WindowMoveDefaultSlidesWithAStrongDecelerate)
{
    auto const def = default_for(miracle::AnimateableEvent::window_move);
    EXPECT_THAT(def.duration_seconds, FloatEq(0.15f));
    ASSERT_THAT(def.data.size(), Eq(1u));
    EXPECT_THAT(def.data[0].type, Eq(miracle::BultInAnimationType::slide));
    EXPECT_THAT(def.data[0].function, Eq(miracle::EaseFunction::ease_out_quart));
}

TEST_F(AnimationConfigurationTest, WindowCloseDefaultShrinksAndFadesOut)
{
    auto const def = default_for(miracle::AnimateableEvent::window_close);
    EXPECT_THAT(def.duration_seconds, FloatEq(0.15f));
    ASSERT_THAT(def.data.size(), Eq(2u));

    EXPECT_THAT(def.data[0].type, Eq(miracle::BultInAnimationType::shrink));
    EXPECT_THAT(def.data[0].function, Eq(miracle::EaseFunction::ease_in_quad));
    EXPECT_THAT(def.data[0].scale, FloatEq(0.92f));

    EXPECT_THAT(def.data[1].type, Eq(miracle::BultInAnimationType::fade));
    EXPECT_THAT(def.data[1].function, Eq(miracle::EaseFunction::ease_in_quad));
}

TEST_F(AnimationConfigurationTest, WorkspaceSwitchDefaultSlides)
{
    auto const def = default_for(miracle::AnimateableEvent::workspace_switch);
    EXPECT_THAT(def.duration_seconds, FloatEq(0.22f));
    ASSERT_THAT(def.data.size(), Eq(1u));
    EXPECT_THAT(def.data[0].type, Eq(miracle::BultInAnimationType::slide));
    EXPECT_THAT(def.data[0].function, Eq(miracle::EaseFunction::ease_out_quart));
}

TEST_F(AnimationConfigurationTest, DefaultScalesNeverOvershoot)
{
    // A window is clipped to its own area while animating, so a scale above 1
    // would be cropped by the scissor rather than drawn as an overshoot.
    for (size_t i = 0; i < static_cast<size_t>(miracle::AnimateableEvent::max); i++)
    {
        auto const def = default_for(static_cast<miracle::AnimateableEvent>(i));
        for (auto const& part : def.data)
        {
            EXPECT_LE(part.scale, 1.f)
                << "event " << miracle::animateable_event_strings[i];
        }
    }
}

TEST_F(AnimationConfigurationTest, DefaultDurationsStayAboveTheFrameCountFloor)
{
    // The animator is frame-rate driven; below roughly 6-8 frames (0.10s at
    // 60Hz) a scale or fade reads as a pop rather than as motion.
    for (size_t i = 0; i < static_cast<size_t>(miracle::AnimateableEvent::max); i++)
    {
        auto const def = default_for(static_cast<miracle::AnimateableEvent>(i));
        EXPECT_GE(def.duration_seconds, 0.12f)
            << "event " << miracle::animateable_event_strings[i];
    }
}

TEST_F(AnimationConfigurationTest, ParsesScaleForGrowAndShrink)
{
    write_yaml(
        "animations:\n"
        "  - event: window_open\n"
        "    duration: 0.3\n"
        "    parts:\n"
        "      - type: grow\n"
        "        function: ease_out_quart\n"
        "        scale: 0.75\n");
    auto const result = miracle::load_config(tmp_path.string());
    auto const& def = result.config.animation_definitions.value[static_cast<size_t>(miracle::AnimateableEvent::window_open)];
    ASSERT_THAT(def.data.size(), Eq(1u));
    EXPECT_THAT(def.data[0].scale, FloatEq(0.75f));
}

TEST_F(AnimationConfigurationTest, ScaleDefaultsToZeroWhenAbsent)
{
    write_yaml(
        "animations:\n"
        "  - event: window_open\n"
        "    duration: 0.3\n"
        "    parts:\n"
        "      - type: grow\n"
        "        function: linear\n");
    auto const result = miracle::load_config(tmp_path.string());
    auto const& def = result.config.animation_definitions.value[static_cast<size_t>(miracle::AnimateableEvent::window_open)];
    ASSERT_THAT(def.data.size(), Eq(1u));
    EXPECT_THAT(def.data[0].scale, FloatEq(0.f));
}

TEST_F(AnimationConfigurationTest, ParsesC5)
{
    write_yaml(
        "animations:\n"
        "  - event: window_open\n"
        "    duration: 0.3\n"
        "    parts:\n"
        "      - type: grow\n"
        "        function: ease_in_out_elastic\n"
        "        c5: 2.5\n");
    auto const result = miracle::load_config(tmp_path.string());
    auto const& def = result.config.animation_definitions.value[static_cast<size_t>(miracle::AnimateableEvent::window_open)];
    ASSERT_THAT(def.data.size(), Eq(1u));
    EXPECT_THAT(def.data[0].c5, FloatEq(2.5f));
}

TEST_F(AnimationConfigurationTest, RoundTripsScaleAndC5ThroughSave)
{
    miracle::ConfigData config;
    miracle::AnimationDefinition definition;
    definition.is_default = false;
    definition.duration_seconds = 0.3f;
    miracle::BuiltInAnimationDefinition part {
        .type = miracle::BultInAnimationType::grow,
        .function = miracle::EaseFunction::ease_out_quart,
    };
    part.c5 = 2.5f;
    part.scale = 0.75f;
    definition.data.push_back(part);
    config.animation_definitions.value[static_cast<size_t>(miracle::AnimateableEvent::window_open)]
        = definition;

    auto const save_result = miracle::save_config(tmp_path.string(), config);
    ASSERT_TRUE(save_result.success);

    auto const load_result = miracle::load_config(tmp_path.string());
    auto const& reloaded = load_result.config.animation_definitions.value[static_cast<size_t>(miracle::AnimateableEvent::window_open)];
    ASSERT_THAT(reloaded.data.size(), Eq(1u));
    EXPECT_THAT(reloaded.data[0].scale, FloatEq(0.75f));
    EXPECT_THAT(reloaded.data[0].c5, FloatEq(2.5f));
}
