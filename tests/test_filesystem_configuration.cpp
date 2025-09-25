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

#include "config.h"
#include "config_observer.h"
#include "yaml-cpp/yaml.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gmock/gmock-function-mocker.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <miracle/animation_definition.h>
#include <miral/runner.h>
#include <vector>
#include <yaml-cpp/node/node.h>

using namespace miracle;

namespace
{
const std::string path = std::filesystem::current_path() / "test.yaml";
}

class FilesystemConfigurationTest : public testing::Test
{
public:
    void SetUp() override
    {
        std::ofstream ofs;
        ofs.open(path, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    void TearDown() override
    {
        std::filesystem::remove(path.c_str());
    }

    void write_kvp(std::string key, std::string value)
    {
        std::fstream file(path, std::ios::app);
        file << key << ": " << value << "\n";
    }

    void write_yaml_node(YAML::Node const& node)
    {
        std::fstream file(path, std::ios::app);
        file << node;
    }

    std::shared_ptr<ConfigObserverRegistrar> registrar = std::make_shared<ConfigObserverRegistrar>();
};

TEST_F(FilesystemConfigurationTest, ConfigurationLoadingDoesNotFailWhenFileDoesNotExist)
{
    std::filesystem::remove(path.c_str());
    EXPECT_NO_THROW(FilesystemConfiguration config(registrar, path, true));
}

TEST_F(FilesystemConfigurationTest, ConfigurationLoadingDoesNotFailWhenFileDoesNotContainYaml)
{
    std::fstream file(path, std::ios::app);
    file << "Hello my name is Matthew { \"fifteen\": 15 } Goodbye then!";
    EXPECT_NO_THROW(FilesystemConfiguration config(registrar, path, true));
}

TEST_F(FilesystemConfigurationTest, DefaultModifierIsMeta)
{
    FilesystemConfiguration config(registrar, path, true);
    ASSERT_EQ(config.get_input_event_modifier(), mir_input_event_modifier_meta);
}

TEST_F(FilesystemConfigurationTest, CanWriteDefaultModifier)
{
    write_kvp("action_key", "alt");
    FilesystemConfiguration config(registrar, path, true);
    ASSERT_EQ(config.get_input_event_modifier(), mir_input_event_modifier_alt);
}

TEST_F(FilesystemConfigurationTest, UnknownModifiersResultsInMeta)
{
    write_kvp("action_key", "unknown");
    FilesystemConfiguration config(registrar, path, true);
    ASSERT_EQ(config.get_input_event_modifier(), mir_input_event_modifier_meta);
}

TEST_F(FilesystemConfigurationTest, WhenDefaultActionOverridesIsNotArrayThenWeDoNotFail)
{
    write_kvp("default_action_overrides", "hello");
    EXPECT_NO_THROW(FilesystemConfiguration config(registrar, path, true));
}

TEST_F(FilesystemConfigurationTest, CanOverrideDefaultAction)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["name"] = "terminal";
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["default_action_overrides"].push_back(action_override_node);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    config.matches_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_X,
        mir_input_event_modifier_meta,
        [&](DefaultKeyCommand command)
    {
        EXPECT_EQ(DefaultKeyCommand::Terminal, command);
        return true;
    });
}

TEST_F(FilesystemConfigurationTest, WhenEntryInDefaultActionOverridesHasInvalidNameThenItIsNotAdded)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["name"].push_back("terminal");
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["default_action_overrides"].push_back(action_override_node);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    config.matches_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_ENTER,
        mir_input_event_modifier_meta,
        [&](DefaultKeyCommand command)
    {
        EXPECT_EQ(DefaultKeyCommand::Terminal, command);
        return true;
    });
}

TEST_F(FilesystemConfigurationTest, WhenEntryInDefaultActionOverridesHasInvalidModifiersThenItIsNotAdded)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["name"] = "terminal";
    action_override_node["action"] = "down";
    action_override_node["modifiers"] = "primary";
    action_override_node["key"] = "KEY_X";
    node["default_action_overrides"].push_back(action_override_node);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    config.matches_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_ENTER,
        mir_input_event_modifier_meta,
        [&](DefaultKeyCommand command)
    {
        EXPECT_EQ(DefaultKeyCommand::Terminal, command);
        return true;
    });
}

TEST_F(FilesystemConfigurationTest, CanCreateCustomAction)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["command"] = "echo Hi";
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["custom_actions"].push_back(action_override_node);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    auto custom_action = config.matches_custom_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_X,
        mir_input_event_modifier_meta);
    EXPECT_EQ(custom_action->command, "echo Hi");
    EXPECT_EQ(custom_action->key, KEY_X);
    EXPECT_EQ(custom_action->action, mir_keyboard_action_down);
}

TEST_F(FilesystemConfigurationTest, CustomActionsInSnapIncludeUnsnapCommand)
{
    setenv("SNAP", "test", 1);
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["command"] = "echo Hi";
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["custom_actions"].push_back(action_override_node);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    auto custom_action = config.matches_custom_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_X,
        mir_input_event_modifier_meta);
    EXPECT_EQ(custom_action->command, "echo Hi");
    unsetenv("SNAP");
}

TEST_F(FilesystemConfigurationTest, CustomActionWithInvalidCommandIsNotAdded)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["command"].push_back("echo Hi");
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["custom_actions"].push_back(action_override_node);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    auto custom_action = config.matches_custom_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_X,
        mir_input_event_modifier_meta);
    EXPECT_EQ(custom_action, nullptr);
}

TEST_F(FilesystemConfigurationTest, InvalidInnerGapsResolveToDefault)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = "hello";
    vec["y"] = "goodbye";
    node["inner_gaps"] = vec;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_inner_gaps().left, 10);
    EXPECT_EQ(config.get_inner_gaps().top, 10);
}

TEST_F(FilesystemConfigurationTest, ValidInnerGapsAreSetCorrectly)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = 33;
    vec["y"] = 44;
    node["inner_gaps"] = vec;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_inner_gaps().left, 33);
    EXPECT_EQ(config.get_inner_gaps().top, 44);
}

TEST_F(FilesystemConfigurationTest, InvalidOuterGapsResolveToDefault)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = "hello";
    vec["y"] = "goodbye";
    node["outer_gaps"] = vec;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_outer_gaps().left, 10);
    EXPECT_EQ(config.get_outer_gaps().top, 10);
}

TEST_F(FilesystemConfigurationTest, ValidOuterGapsAreSetCorrectly)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = 33;
    vec["y"] = 44;
    node["outer_gaps"] = vec;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_outer_gaps().left, 33);
    EXPECT_EQ(config.get_outer_gaps().top, 44);
}

TEST_F(FilesystemConfigurationTest, ValidStartupAppsAreParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"] = "echo Hi";
    startup_app["restart_on_death"] = true;
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_startup_apps().size(), 1);
    EXPECT_EQ(config.get_startup_apps()[0].command, "echo Hi");
    EXPECT_EQ(config.get_startup_apps()[0].restart_on_death, true);
}

TEST_F(FilesystemConfigurationTest, StartupAppsInSnapIncludeUnsnapCommand)
{
    setenv("SNAP", "test", 1);
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"] = "echo Hi";
    startup_app["restart_on_death"] = true;
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_startup_apps()[0].command, "echo Hi");
    unsetenv("SNAP");
}

TEST_F(FilesystemConfigurationTest, StartupAppsThatIsNotAnArrayIsNotParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    node["startup_apps"] = "Hello";
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_startup_apps().size(), 0);
}

TEST_F(FilesystemConfigurationTest, StartupAppsInvalidCommandIsNotParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"].push_back("Hello");
    startup_app["restart_on_death"] = false;
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_startup_apps().size(), 0);
}

TEST_F(FilesystemConfigurationTest, StartupAppsInvalidRestartOnDeathIsNotParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"] = "echo Hi";
    startup_app["restart_on_death"] = "Hello";
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_startup_apps().size(), 0);
}

TEST_F(FilesystemConfigurationTest, EnvironmentVariableInvalidWhenKeyIsMissing)
{
    YAML::Node node;
    YAML::Node environment_variable;
    environment_variable["value"] = "1";
    node["environment_variables"].push_back(environment_variable);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_env_variables().size(), 0);
}

TEST_F(FilesystemConfigurationTest, EnvironmentVariableInvalidWhenValueIsMissing)
{
    YAML::Node node;
    YAML::Node environment_variable;
    environment_variable["key"] = "my_key";
    node["environment_variables"].push_back(environment_variable);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_env_variables().size(), 0);
}

TEST_F(FilesystemConfigurationTest, EnvironmentVariableCanBeParsed)
{
    YAML::Node node;
    YAML::Node environment_variable;
    environment_variable["key"] = "my_key";
    environment_variable["value"] = "1";
    node["environment_variables"].push_back(environment_variable);
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_env_variables().size(), 1);
}

TEST_F(FilesystemConfigurationTest, BorderCanBeParsedWithArrayColors)
{
    YAML::Node border;
    border["size"] = 2;

    YAML::Node color_node;
    color_node.push_back(255);
    color_node.push_back(155);
    color_node.push_back(155);
    color_node.push_back(255);
    border["color"] = color_node;

    YAML::Node focus_color;
    focus_color.push_back(10);
    focus_color.push_back(10);
    focus_color.push_back(10);
    focus_color.push_back(255);
    border["focus_color"] = focus_color;

    YAML::Node node;
    node["border"] = border;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_border_config().size, 2);
    EXPECT_EQ(config.get_border_config().color.r, 1.f);
    EXPECT_EQ(config.get_border_config().color.g, 155.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.b, 155.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.a, 1.f);
    EXPECT_EQ(config.get_border_config().focus_color.r, 10.f / 255.f);
    EXPECT_EQ(config.get_border_config().focus_color.g, 10.f / 255.f);
    EXPECT_EQ(config.get_border_config().focus_color.b, 10.f / 255.f);
    EXPECT_EQ(config.get_border_config().focus_color.a, 1.f);
}

TEST_F(FilesystemConfigurationTest, BorderCanBeParsedWithHexColor)
{
    YAML::Node border;
    border["size"] = 2;

    border["color"] = "0xDD89DDFF";

    YAML::Node focus_color;
    focus_color.push_back(10);
    focus_color.push_back(10);
    focus_color.push_back(10);
    focus_color.push_back(255);
    border["focus_color"] = focus_color;

    YAML::Node node;
    node["border"] = border;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_border_config().color.r, 221.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.g, 137.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.b, 221.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.a, 1.f);
}

TEST_F(FilesystemConfigurationTest, BorderCanBeParsedObjectColor)
{
    YAML::Node border;
    border["size"] = 2;

    border["color"]["r"] = 15;
    border["color"]["g"] = 25;
    border["color"]["b"] = 30;
    border["color"]["a"] = 55;

    YAML::Node focus_color;
    focus_color.push_back(10);
    focus_color.push_back(10);
    focus_color.push_back(10);
    focus_color.push_back(255);
    border["focus_color"] = focus_color;

    YAML::Node node;
    node["border"] = border;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.get_border_config().color.r, 15.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.g, 25.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.b, 30.f / 255.f);
    EXPECT_EQ(config.get_border_config().color.a, 55.f / 255.f);
}

TEST_F(FilesystemConfigurationTest, DragAndDropAllValues)
{
    YAML::Node drag_and_drop;
    drag_and_drop["enabled"] = true;
    drag_and_drop["modifiers"].push_back("alt");

    YAML::Node node;
    node["drag_and_drop"] = drag_and_drop;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.drag_and_drop().enabled, true);
    EXPECT_EQ(config.drag_and_drop().modifiers, mir_input_event_modifier_alt);
}

TEST_F(FilesystemConfigurationTest, DragAndDropMissingEnabled)
{
    YAML::Node drag_and_drop;
    drag_and_drop["modifiers"].push_back("primary");

    YAML::Node node;
    node["drag_and_drop"] = drag_and_drop;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.drag_and_drop().enabled, true);
    EXPECT_EQ(config.drag_and_drop().modifiers, miracle_input_event_modifier_default);
}

TEST_F(FilesystemConfigurationTest, DragAndDropMissingModifiers)
{
    YAML::Node drag_and_drop;
    drag_and_drop["enabled"] = true;

    YAML::Node node;
    node["drag_and_drop"] = drag_and_drop;
    write_yaml_node(node);

    FilesystemConfiguration config(registrar, path, true);
    EXPECT_EQ(config.drag_and_drop().enabled, true);
    EXPECT_EQ(config.drag_and_drop().modifiers, miracle_input_event_modifier_default | mir_input_event_modifier_shift);
}

struct AnimationTypeParam
{
    std::string value;
    BultInAnimationType expected;
};

class FilesystemConfigurationTestAnimationTypes : public FilesystemConfigurationTest, public ::testing::WithParamInterface<AnimationTypeParam>
{
};

TEST_P(FilesystemConfigurationTestAnimationTypes, CanReadAnimationType)
{
    auto param = GetParam();
    YAML::Node animations_node;
    YAML::Node animation;
    animation["event"] = "window_open";
    animation["type"] = param.value;
    animation["function"] = "linear";
    animations_node.push_back(animation);

    YAML::Node root;
    root["animations"] = animations_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto def = config.get_animation_definition(AnimateableEvent::window_open);
    EXPECT_EQ(def.animations[0].type, param.expected);
}

TEST_P(FilesystemConfigurationTestAnimationTypes, CanReadAnimationTypeInAnimationList)
{
    auto param = GetParam();
    YAML::Node list_item;
    list_item["type"] = param.value;
    list_item["function"] = "linear";

    YAML::Node list;
    list.push_back(list_item);

    YAML::Node animation;
    animation["duration"] = 1000;
    animation["event"] = "window_open";
    animation["list"] = list;

    YAML::Node animations_node;
    animations_node.push_back(animation);

    YAML::Node root;
    root["animations"] = animations_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto def = config.get_animation_definition(AnimateableEvent::window_open);
    EXPECT_EQ(def.animations[0].type, param.expected);
}

TEST_F(FilesystemConfigurationTest, CanReadSimulatedSecondaryClick)
{
    YAML::Node ssc_node;
    ssc_node["enabled"] = true;
    ssc_node["hold_duration"] = 123;
    ssc_node["displacement_threshold"] = 456;

    YAML::Node root;
    root["simulated_secondary_click"] = ssc_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto const ssc = config.simulated_secondary_click();
    EXPECT_THAT(ssc.enabled, testing::Eq(true));
    EXPECT_THAT(ssc.hold_duration_milliseconds, testing::Eq(123));
    EXPECT_THAT(ssc.displacement_threshold, testing::Eq(456));
}

INSTANTIATE_TEST_SUITE_P(
    FilesystemConfigurationTestAnimationTypes,
    FilesystemConfigurationTestAnimationTypes,
    ::testing::Values(
        AnimationTypeParam("disabled", BultInAnimationType::disabled),
        AnimationTypeParam("slide", BultInAnimationType::slide),
        AnimationTypeParam("grow", BultInAnimationType::grow),
        AnimationTypeParam("shrink", BultInAnimationType::shrink),
        AnimationTypeParam("fade_in", BultInAnimationType::fade_in),
        AnimationTypeParam("fade_out", BultInAnimationType::fade_out)));

TEST_F(FilesystemConfigurationTest, TriggersListenerOnReload)
{
    class Observer : public ConfigObserver
    {
    public:
        MOCK_METHOD((void), on_config_changed, (Config const&), (override));
    };

    auto const observer = std::make_shared<Observer>();
    registrar->register_interest(observer);
    EXPECT_CALL(*observer, on_config_changed).Times(1);

    FilesystemConfiguration config(registrar, path, true);
}

TEST_F(FilesystemConfigurationTest, CanReadOutputFilter)
{
    YAML::Node output_filter_node;
    output_filter_node["shader_path"] = "hello.frag";

    YAML::Node root;
    root["output_filter"] = output_filter_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto const output_filter = config.output_filter();
    EXPECT_THAT(output_filter.shader_path.value(), testing::Eq("hello.frag"));
}

TEST_F(FilesystemConfigurationTest, CanReadOutputFilterWithTilde)
{
    setenv("HOME", "/home/user", 1);
    YAML::Node output_filter_node;
    output_filter_node["shader_path"] = "~/hello.frag";

    YAML::Node root;
    root["output_filter"] = output_filter_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto const output_filter = config.output_filter();
    EXPECT_THAT(output_filter.shader_path.value(), testing::Eq("/home/user/hello.frag"));
}

TEST_F(FilesystemConfigurationTest, CanReadCursor)
{
    YAML::Node cursor_node;
    cursor_node["scale"] = 4.f;

    YAML::Node root;
    root["cursor"] = cursor_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto const cursor = config.cursor();
    EXPECT_THAT(cursor.scale, testing::Eq(4.f));
}

TEST_F(FilesystemConfigurationTest, CanReadSlowKeys)
{
    YAML::Node slow_keys_node;
    slow_keys_node["enabled"] = true;
    slow_keys_node["hold_delay"] = 500;

    YAML::Node root;
    root["slow_keys"] = slow_keys_node;
    write_yaml_node(root);

    FilesystemConfiguration config(registrar, path, true);
    auto const slow_keys = config.slow_keys();
    EXPECT_THAT(slow_keys.enabled, testing::Eq(true));
    EXPECT_THAT(slow_keys.hold_delay_milliseconds, testing::Eq(500));
}
