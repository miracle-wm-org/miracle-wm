#include <gtest/gtest.h>
#include "miracle_config.h"
#include <miral/runner.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include "yaml-cpp/yaml.h"
#include <cstdlib>

using namespace miracle;

namespace
{
int argc = 1;
char const* argv[] = {"miracle-wm-tests"};
const std::string path = std::filesystem::current_path() / "test.yaml";
}

class MiracleConfigTest : public testing::Test
{
public:
    MiracleConfigTest() : runner(argc, argv)
    {
    }

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

    miral::MirRunner runner;
};

TEST_F(MiracleConfigTest, ConfigurationLoadingDoesNotFailWhenFileDoesNotExist)
{
    std::filesystem::remove(path.c_str());
    EXPECT_NO_THROW(MiracleConfig config(runner, path));
}

TEST_F(MiracleConfigTest, ConfigurationLoadingDoesNotFailWhenFileDoesNotContainYaml)
{
    std::fstream file(path, std::ios::app);
    file << "Hello my name is Matthew { \"fifteen\": 15 } Goodbye then!";
    EXPECT_NO_THROW(MiracleConfig config(runner, path));
}

TEST_F(MiracleConfigTest, DefaultModifierIsMeta)
{
    MiracleConfig config(runner, path);
    ASSERT_EQ(config.get_input_event_modifier(), mir_input_event_modifier_meta);
}

TEST_F(MiracleConfigTest, CanWriteDefaultModifier)
{
    write_kvp("action_key", "alt");
    MiracleConfig config(runner, path);
    ASSERT_EQ(config.get_input_event_modifier(), mir_input_event_modifier_alt);
}

TEST_F(MiracleConfigTest, UnknownModifiersResultsInMeta)
{
    write_kvp("action_key", "unknown");
    MiracleConfig config(runner, path);
    ASSERT_EQ(config.get_input_event_modifier(), mir_input_event_modifier_meta);
}

TEST_F(MiracleConfigTest, WhenDefaultActionOverridesIsNotArrayThenWeDoNotFail)
{
    write_kvp("default_action_overrides", "hello");
    EXPECT_NO_THROW(MiracleConfig config(runner, path));
}

TEST_F(MiracleConfigTest, CanOverrideDefaultAction)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["name"] = "terminal";
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["default_action_overrides"].push_back(action_override_node);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(
        config.matches_key_command(
            MirKeyboardAction::mir_keyboard_action_down,
            KEY_X,
            mir_input_event_modifier_meta),
        Terminal);
}

TEST_F(MiracleConfigTest, WhenEntryInDefaultActionOverridesHasInvalidNameThenItIsNotAdded)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["name"].push_back("terminal");
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["default_action_overrides"].push_back(action_override_node);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(
        config.matches_key_command(
            MirKeyboardAction::mir_keyboard_action_down,
            KEY_ENTER,
            mir_input_event_modifier_meta),
        Terminal);
}

TEST_F(MiracleConfigTest, WhenEntryInDefaultActionOverridesHasInvalidModifiersThenItIsNotAdded)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["name"] = "terminal";
    action_override_node["action"] = "down";
    action_override_node["modifiers"] = "primary";
    action_override_node["key"] = "KEY_X";
    node["default_action_overrides"].push_back(action_override_node);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(
        config.matches_key_command(
            MirKeyboardAction::mir_keyboard_action_down,
            KEY_ENTER,
            mir_input_event_modifier_meta),
        Terminal);
}

TEST_F(MiracleConfigTest, CanCreateCustomAction)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["command"] = "echo Hi";
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["custom_actions"].push_back(action_override_node);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    auto custom_action = config.matches_custom_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_X,
        mir_input_event_modifier_meta);
    EXPECT_EQ(custom_action->command, "echo Hi");
    EXPECT_EQ(custom_action->key, KEY_X);
    EXPECT_EQ(custom_action->action, mir_keyboard_action_down);
}

TEST_F(MiracleConfigTest, CustomActionsInSnapIncludeUnsnapCommand)
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

    MiracleConfig config(runner, path);
    auto custom_action = config.matches_custom_key_command(
    MirKeyboardAction::mir_keyboard_action_down,
    KEY_X,
    mir_input_event_modifier_meta);
    EXPECT_EQ(custom_action->command, "miracle-wm-unsnap echo Hi");
    unsetenv("SNAP");
}

TEST_F(MiracleConfigTest, CustomActionWithInvalidCommandIsNotAdded)
{
    YAML::Node node;
    YAML::Node action_override_node;
    action_override_node["command"].push_back("echo Hi");
    action_override_node["action"] = "down";
    action_override_node["modifiers"].push_back("primary");
    action_override_node["key"] = "KEY_X";
    node["custom_actions"].push_back(action_override_node);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    auto custom_action = config.matches_custom_key_command(
        MirKeyboardAction::mir_keyboard_action_down,
        KEY_X,
        mir_input_event_modifier_meta);
    EXPECT_EQ(custom_action, nullptr);
}

TEST_F(MiracleConfigTest, InvalidInnerGapsResolveToDefault)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = "hello";
    vec["y"] = "goodbye";
    node["inner_gaps"] = vec;
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_inner_gaps_x(), 10);
    EXPECT_EQ(config.get_inner_gaps_y(), 10);
}

TEST_F(MiracleConfigTest, ValidInnerGapsAreSetCorrectly)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = 33;
    vec["y"] = 44;
    node["inner_gaps"] = vec;
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_inner_gaps_x(), 33);
    EXPECT_EQ(config.get_inner_gaps_y(), 44);
}

TEST_F(MiracleConfigTest, InvalidOuterGapsResolveToDefault)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = "hello";
    vec["y"] = "goodbye";
    node["outer_gaps"] = vec;
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_outer_gaps_x(), 10);
    EXPECT_EQ(config.get_outer_gaps_y(), 10);
}

TEST_F(MiracleConfigTest, ValidOuterGapsAreSetCorrectly)
{
    YAML::Node node;
    YAML::Node vec;
    vec["x"] = 33;
    vec["y"] = 44;
    node["outer_gaps"] = vec;
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_outer_gaps_x(), 33);
    EXPECT_EQ(config.get_outer_gaps_y(), 44);
}

TEST_F(MiracleConfigTest, ValidStartupAppsAreParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"] = "echo Hi";
    startup_app["restart_on_death"] = true;
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_startup_apps().size(), 1);
    EXPECT_EQ(config.get_startup_apps()[0].command, "echo Hi");
    EXPECT_EQ(config.get_startup_apps()[0].restart_on_death, true);
}

TEST_F(MiracleConfigTest, StartupAppsInSnapIncludeUnsnapCommand)
{
    setenv("SNAP", "test", 1);
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"] = "echo Hi";
    startup_app["restart_on_death"] = true;
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_startup_apps()[0].command, "miracle-wm-unsnap echo Hi");
    unsetenv("SNAP");
}

TEST_F(MiracleConfigTest, StartupAppsThatIsNotAnArrayIsNotParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    node["startup_apps"] = "Hello";
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_startup_apps().size(), 0);
}

TEST_F(MiracleConfigTest, StartupAppsInvalidCommandIsNotParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"].push_back("Hello");
    startup_app["restart_on_death"] = false;
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_startup_apps().size(), 0);
}

TEST_F(MiracleConfigTest, StartupAppsInvalidRestartOnDeathIsNotParsed)
{
    YAML::Node node;
    YAML::Node startup_app;
    startup_app["command"] = "echo Hi";
    startup_app["restart_on_death"] = "Hello";
    node["startup_apps"].push_back(startup_app);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_startup_apps().size(), 0);
}

TEST_F(MiracleConfigTest, EnvironmentVariableInvalidWhenKeyIsMissing)
{
    YAML::Node node;
    YAML::Node environment_variable;
    environment_variable["value"] = "1";
    node["environment_variables"].push_back(environment_variable);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_env_variables().size(), 0);
}

TEST_F(MiracleConfigTest, EnvironmentVariableInvalidWhenValueIsMissing)
{
    YAML::Node node;
    YAML::Node environment_variable;
    environment_variable["key"] = "my_key";
    node["environment_variables"].push_back(environment_variable);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_env_variables().size(), 0);
}

TEST_F(MiracleConfigTest, EnvironmentVariableCanBeParsed)
{
    YAML::Node node;
    YAML::Node environment_variable;
    environment_variable["key"] = "my_key";
    environment_variable["value"] = "1";
    node["environment_variables"].push_back(environment_variable);
    write_yaml_node(node);

    MiracleConfig config(runner, path);
    EXPECT_EQ(config.get_env_variables().size(), 1);
}