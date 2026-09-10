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

#include "display_config.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <mir/graphics/display_configuration.h>
#include <mir/test/doubles/stub_display_configuration.h>
#include <mir_toolkit/common.h>

using namespace miracle;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
const std::string config_path = std::filesystem::temp_directory_path() / "miracle_test_display_config.yaml";

void write_display_yaml(std::string const& content)
{
    std::ofstream f(config_path);
    f << content;
}

mtd::StubDisplayConfigurationOutput make_output(int id, std::string const& name, int width, int height)
{
    mtd::StubDisplayConfigurationOutput output(
        mg::DisplayConfigurationOutputId {
            id
    },
        { { { width, height }, 60.0 } },
        { mir_pixel_format_abgr_8888 });
    output.name = name;
    return output;
}
}

class DisplayConfigTest : public testing::Test
{
public:
    void TearDown() override
    {
        std::filesystem::remove(config_path);
    }
};

// When a monitor is plugged in that is not in the display config, it should be enabled.
TEST_F(DisplayConfigTest, UnconfiguredMonitorIsEnabled)
{
    write_display_yaml(
        "outputs:\n"
        "  - name: \"HDMI-1\"\n"
        "    enabled: true\n"
        "    position:\n"
        "      x: 0\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n");

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    EXPECT_TRUE(display_conf.outputs[1].used);
    EXPECT_EQ(display_conf.outputs[1].power_mode, mir_power_mode_on);
}

// An unconfigured monitor should be placed immediately to the right of the
// rightmost configured monitor.
TEST_F(DisplayConfigTest, UnconfiguredMonitorPlacedAfterConfigured)
{
    write_display_yaml(
        "outputs:\n"
        "  - name: \"HDMI-1\"\n"
        "    enabled: true\n"
        "    position:\n"
        "      x: 0\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n");

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    EXPECT_EQ(display_conf.outputs[1].top_left.x.as_int(), 1920);
    EXPECT_EQ(display_conf.outputs[1].top_left.y.as_int(), 0);
}

// Multiple unconfigured monitors should be placed sequentially to the right
// of all configured monitors.
TEST_F(DisplayConfigTest, MultipleUnconfiguredMonitorsPlacedSequentially)
{
    write_display_yaml(
        "outputs:\n"
        "  - name: \"HDMI-1\"\n"
        "    enabled: true\n"
        "    position:\n"
        "      x: 0\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n");

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({
        make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080),
        make_output(3, "HDMI-3", 1920, 1080),
    });

    config.apply_to_config(display_conf);

    EXPECT_EQ(display_conf.outputs[1].top_left.x.as_int(), 1920);
    EXPECT_EQ(display_conf.outputs[2].top_left.x.as_int(), 3840);
}

// A configured monitor's position should not be affected by the presence of
// unconfigured monitors.
TEST_F(DisplayConfigTest, ConfiguredMonitorPositionUnaffected)
{
    write_display_yaml(
        "outputs:\n"
        "  - name: \"HDMI-1\"\n"
        "    enabled: true\n"
        "    position:\n"
        "      x: 0\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n");

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    EXPECT_TRUE(display_conf.outputs[0].used);
    EXPECT_EQ(display_conf.outputs[0].top_left.x.as_int(), 0);
    EXPECT_EQ(display_conf.outputs[0].top_left.y.as_int(), 0);
}

// An output marked as disabled in the display config should be turned off.
TEST_F(DisplayConfigTest, DisabledMonitorIsTurnedOff)
{
    write_display_yaml(
        "outputs:\n"
        "  - name: \"HDMI-1\"\n"
        "    enabled: true\n"
        "    position:\n"
        "      x: 0\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n"
        "  - name: \"HDMI-2\"\n"
        "    enabled: false\n"
        "    position:\n"
        "      x: 1920\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n");

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    EXPECT_TRUE(display_conf.outputs[0].used);
    EXPECT_EQ(display_conf.outputs[0].power_mode, mir_power_mode_on);
    EXPECT_FALSE(display_conf.outputs[1].used);
    EXPECT_EQ(display_conf.outputs[1].power_mode, mir_power_mode_off);
}

// Disabling every output would leave the user with nothing to look at, so the
// request is refused and all outputs stay enabled.
TEST_F(DisplayConfigTest, DisablingEveryMonitorIsRefused)
{
    write_display_yaml(
        "outputs:\n"
        "  - name: \"HDMI-1\"\n"
        "    enabled: false\n"
        "    position:\n"
        "      x: 0\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n"
        "  - name: \"HDMI-2\"\n"
        "    enabled: false\n"
        "    position:\n"
        "      x: 1920\n"
        "      y: 0\n"
        "    size:\n"
        "      width: 1920\n"
        "      height: 1080\n"
        "    scale: 1\n"
        "    group_id: 0\n"
        "    orientation: normal\n");

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    EXPECT_TRUE(display_conf.outputs[0].used);
    EXPECT_EQ(display_conf.outputs[0].power_mode, mir_power_mode_on);
    EXPECT_TRUE(display_conf.outputs[1].used);
    EXPECT_EQ(display_conf.outputs[1].power_mode, mir_power_mode_on);
}

namespace
{
class RecordingListener : public DisplayConfigListener
{
public:
    void display_configuration_changed(OutputConfigDetailList const& configuration) override
    {
        last = configuration;
        count++;
    }

    OutputConfigDetailList last;
    int count = 0;
};

std::string const one_enabled_one_disabled = "outputs:\n"
                                             "  - name: \"HDMI-1\"\n"
                                             "    enabled: true\n"
                                             "    position:\n"
                                             "      x: 0\n"
                                             "      y: 0\n"
                                             "    size:\n"
                                             "      width: 1920\n"
                                             "      height: 1080\n"
                                             "    scale: 1\n"
                                             "    group_id: 0\n"
                                             "    orientation: normal\n"
                                             "  - name: \"HDMI-2\"\n"
                                             "    enabled: false\n"
                                             "    position:\n"
                                             "      x: 1920\n"
                                             "      y: 0\n"
                                             "    size:\n"
                                             "      width: 1920\n"
                                             "      height: 1080\n"
                                             "    scale: 1\n"
                                             "    group_id: 0\n"
                                             "    orientation: normal\n";
}

// A disabled output must remain in the snapshot that the wlr-output-management
// extension reads, otherwise its head would be dropped instead of advertised as
// "enabled: 0".
TEST_F(DisplayConfigTest, DisabledMonitorRemainsInTheConfigurationSnapshot)
{
    write_display_yaml(one_enabled_one_disabled);

    DisplayConfig config(config_path);
    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    auto const configuration = config.configuration();
    ASSERT_EQ(configuration.size(), 2);
    EXPECT_EQ(configuration[0].name, "HDMI-1");
    EXPECT_TRUE(configuration[0].connected);
    EXPECT_TRUE(configuration[0].used);
    EXPECT_EQ(configuration[1].name, "HDMI-2");
    EXPECT_TRUE(configuration[1].connected);
    EXPECT_FALSE(configuration[1].used);
}

// Listeners are told about the configuration, including the disabled outputs.
TEST_F(DisplayConfigTest, ListenersAreNotifiedOfDisabledMonitors)
{
    write_display_yaml(one_enabled_one_disabled);

    DisplayConfig config(config_path);
    auto const listener = std::make_shared<RecordingListener>();
    config.register_listener(listener);

    mtd::StubDisplayConfig display_conf({ make_output(1, "HDMI-1", 1920, 1080),
        make_output(2, "HDMI-2", 1920, 1080) });

    config.apply_to_config(display_conf);

    EXPECT_GT(listener->count, 0);
    ASSERT_EQ(listener->last.size(), 2);
    EXPECT_TRUE(listener->last[0].used);
    EXPECT_FALSE(listener->last[1].used);
    EXPECT_TRUE(listener->last[1].connected);
}
