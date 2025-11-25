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

#define MIR_LOG_COMPONENT "display_config"

#include "display_config.h"

#include "miracle/cpp/config-cpp.h"
#include "output.h"

#include <filesystem>
#include <fstream>
#include <mir/graphics/display_configuration_policy.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <mir/shell/display_configuration_controller.h>
#include <mutex>
#include <yaml-cpp/yaml.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
auto select_mode_index(uint32_t mode_index, std::vector<mg::DisplayConfigurationMode> const& modes) -> uint32_t
{
    if (modes.empty())
        return std::numeric_limits<uint32_t>::max();

    if (mode_index >= modes.size())
        return 0;

    return mode_index;
}
}

namespace YAML
{
template <>
struct convert<mir::geometry::Point>
{
    static Node encode(const mir::geometry::Point& p)
    {
        Node node;
        node["x"] = p.x.as_int();
        node["y"] = p.y.as_int();
        return node;
    }

    static bool decode(const Node& node, mir::geometry::Point& p)
    {
        if (!node.IsMap() || !node["x"] || !node["y"])
            return false;
        p.x = geom::X { node["x"].as<int>() };
        p.y = geom::Y { node["y"].as<int>() };
        return true;
    }
};

template <>
struct convert<mir::geometry::Size>
{
    static Node encode(const mir::geometry::Size& s)
    {
        Node node;
        node["width"] = s.width.as_int();
        node["height"] = s.height.as_int();
        return node;
    }

    static bool decode(const Node& node, mir::geometry::Size& s)
    {
        if (!node.IsMap() || !node["width"] || !node["height"])
            return false;
        s.width = geom::Width { node["width"].as<int>() };
        s.height = geom::Height { node["height"].as<int>() };
        return true;
    }
};

template <>
struct convert<miracle::DisplayConfig::OutputConfig>
{
    static Node encode(const miracle::DisplayConfig::OutputConfig& card)
    {
        Node node;
        node["enabled"] = card.enabled;
        node["name"] = card.name;
        if (card.primary)
            node["primary"] = card.primary;
        if (card.position)
            node["position"] = *card.position;
        if (card.size)
            node["size"] = *card.size;
        if (card.refresh)
            node["refresh"] = *card.refresh;
        switch (card.orientation)
        {
        case mir_orientation_normal:
            node["orientation"] = "normal";
            break;
        case mir_orientation_left:
            node["orientation"] = "left";
            break;
        case mir_orientation_inverted:
            node["orientation"] = "inverted";
            break;
        case mir_orientation_right:
            node["orientation"] = "right";
            break;
        }
        node["scale"] = card.scale;
        node["group_id"] = card.group_id.as_value();
        return node;
    }

    static bool decode(const Node& node, miracle::DisplayConfig::OutputConfig& card)
    {
        card.name = node["name"].as<std::string>();
        card.enabled = node["enabled"].as<bool>(false);

        if (node["primary"])
            card.primary = node["primary"].as<bool>();
        if (node["position"])
            card.position = node["position"].as<mir::geometry::Point>();
        if (node["size"])
            card.size = node["size"].as<mir::geometry::Size>();
        if (node["refresh"])
            card.refresh = node["refresh"].as<double>();

        if (node["orientation"])
        {
            auto const orientation = node["orientation"].as<std::string>();
            if (orientation == "normal")
                card.orientation = mir_orientation_normal;
            else if (orientation == "left")
                card.orientation = mir_orientation_left;
            else if (orientation == "inverted")
                card.orientation = mir_orientation_inverted;
            else if (orientation == "right")
                card.orientation = mir_orientation_right;
        }

        if (node["scale"])
            card.scale = node["scale"].as<float>(1.f);
        if (node["group_id"])
            card.group_id = mg::DisplayConfigurationLogicalGroupId(node["group_id"].as<int>());
        return true;
    }
};
}

class miracle::DisplayConfig::Self : public mg::DisplayConfigurationPolicy
{
public:
    explicit Self(std::string const& path) :
        path(path)
    {
        if (has_config_file())
            try_load_from_file();
    }

    /// Override from [mg::DisplayConfigurationPolicy]
    void apply_to(mg::DisplayConfiguration& conf) override
    {
        // If we have a configuration, we should load and apply it.
        // Otherwise, let's just apply the default.
        if (has_config_file() && try_load_from_file())
            apply_internal(conf, configs);
        else
            apply_default(conf);
    }

    /// Override from [mg::DisplayConfigurationPolicy]
    void confirm(mg::DisplayConfiguration const& conf) override
    {
        std::lock_guard lock(mutex);

        cached.clear();
        conf.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            cached.push_back(OutputConfigDetails {
                output.name,
                output.card_id,
                output.physical_size_mm,
                output.top_left,
                output.scale,
                output.orientation,
                output.logical_group_id,
                output.modes,
                output.current_mode_index,
                output.used,
                output.power_mode,
                output.current_format,
                output.custom_attribute.contains("primary")
                    ? output.custom_attribute.at("primary") == "true"
                    : false,
                output.display_info });
        });

        auto copy = conf.clone();
        if (!has_config_file())
            try_create_default(*copy);
    }

    /// Attempts to write the default configuration to [path] if it does not exist.
    bool try_create_default(mg::DisplayConfiguration& config) const noexcept
    {
        if (!std::filesystem::exists(path))
        {
            mir::log_info("Writing default display configuration to %s", path.c_str());
            auto const configs_list = to_output_config_list(config);
            YAML::Node node;
            for (auto const& output_config : configs_list)
                node.push_back(output_config);

            YAML::Node container;
            container["outputs"] = node;
            std::ofstream output_file(path);
            output_file << container;
            mir::log_info("Default display configuration written");
            return true;
        }

        return false;
    }

    /// Returns true if [path] exists, otherwise false;
    [[nodiscard]] bool has_config_file() const noexcept
    {
        return std::filesystem::exists(path);
    }

    /// Populates [configs] with whatever is in the file.
    bool try_load_from_file()
    {
        try
        {
            std::lock_guard lock(mutex);
            mir::log_info("Loading display configuration from %s", path.c_str());
            YAML::Node const node = YAML::LoadFile(path);
            if (node["outputs"])
                configs = node["outputs"].as<std::vector<OutputConfig>>();
            else
            {
                mir::log_error("Display configuration file is missing 'outputs' key");
                return false;
            }

            mir::log_info("Display configuration loaded");
            return true;
        }
        catch (const std::exception& e)
        {
            mir::log_error("Exception during DisplayConfig load: %s", e.what());
        }
        catch (...)
        {
            mir::log_error("Unknown exception during DisplayConfig load");
        }
        return false;
    }

    /// Manually applies the configuration in [configs] to the base configuration.
    void apply_configuration_manually() const
    {
        if (auto const dcc = display_configuration_controller.lock())
        {
            mir::log_info("Applying display configuration");
            auto const config = dcc->base_configuration();
            if (configs.empty())
            {
                mir::log_error("Display configuration is empty");
                return;
            }

            apply_internal(*config, configs);
            dcc->set_base_configuration(config);
            mir::log_info("Display configuration applied");
        }
        else
        {
            mir::log_error("Cannot apply display configuration: display configuration controller is not available");
        }
    }

    /// Applies the config given by the parameters [test_configs].
    /// This method does NOT write [test_configs] to file.
    void test(std::vector<OutputConfig> const& test_configs) const
    {
        if (auto const dcc = display_configuration_controller.lock())
        {
            mir::log_info("Testing display configuration");
            auto const config = dcc->base_configuration();
            apply_internal(*config, test_configs);
            dcc->set_base_configuration(config);
            mir::log_info("Display configuration test applied");
        }
        else
        {
            mir::log_error("Cannot test display configuration: display configuration controller is not available");
        }
    }

    /// Writes the configuration currently held in [configs] to file.
    void write()
    {
        std::lock_guard lock(mutex);
        YAML::Node node;
        for (const auto& config : configs)
            node.push_back(config);

        YAML::Node container;
        container["outputs"] = node;
        std::ofstream output_file(path);
        output_file << container;
    }

    /// Inserts or updates the config provided by the parameter.
    void update(OutputConfig const& config)
    {
        std::lock_guard lock(mutex);
        auto const output_it = std::ranges::find_if(configs, [&](OutputConfig const& o)
        {
            return o.name == config.name;
        });

        if (output_it != configs.end())
            *output_it = config;
        else
            configs.push_back(config);
    }

    /// Returns a copy of the [OutputConfig] list.
    std::vector<OutputConfig> get_configs()
    {
        std::lock_guard lock(mutex);
        return configs;
    }

    /// Returns a copy of the set configuration, if it exists, otherwise std::nullopt.
    [[nodiscard]] OutputConfigDetailList configuration()
    {
        std::lock_guard lock(mutex);
        return cached;
    }

    std::weak_ptr<mir::shell::DisplayConfigurationController> display_configuration_controller;
    std::mutex mutex;
    std::string path;

private:
    std::vector<OutputConfig> configs;
    OutputConfigDetailList cached;
    mg::DisplayConfigurationLogicalGroupId const empty_group_id = mg::DisplayConfigurationLogicalGroupId(0);

    void apply_default(mg::DisplayConfiguration& conf) const
    {
        mir::log_info("Applying default display configuration");
        // By default, we place all displays next to each other horizontally.
        geom::Point position(0, 0);

        conf.for_each_output([&](mg::UserDisplayConfigurationOutput& output)
        {
            if (!output.connected || output.modes.empty())
            {
                output.used = false;
                output.power_mode = mir_power_mode_off;
                return;
            }

            output.used = true;
            output.power_mode = mir_power_mode_on;
            output.top_left = position;
            uint32_t const preferred_mode_index { select_mode_index(output.preferred_mode_index, output.modes) };
            output.current_mode_index = preferred_mode_index;
            output.logical_group_id = empty_group_id;
            output.orientation = mir_orientation_normal;
            position.x = geom::X { position.x.as_int() + output.extents().size.width.as_int() };
        });
        mir::log_info("Default display configuration applied");
    }

    static std::vector<OutputConfig> to_output_config_list(mg::DisplayConfiguration& configuration)
    {
        std::vector<OutputConfig> result;
        geom::Point position(0, 0);
        bool enconutered_first = false;
        configuration.for_each_output([&](mg::UserDisplayConfigurationOutput& output)
        {
            if (!output.connected || output.modes.empty())
                return;

            OutputConfig config;
            config.enabled = output.connected && !output.modes.empty();
            config.primary = !enconutered_first;
            config.name = output.name;
            config.position = position;
            position.x = geom::X { position.x.as_int() + output.extents().size.width.as_int() };
            size_t const preferred_mode_index { select_mode_index(output.preferred_mode_index, output.modes) };
            auto const& mode = preferred_mode_index < output.modes.size()
                ? output.modes[preferred_mode_index]
                : output.modes[0];
            config.size = mode.size;
            config.refresh = mode.vrefresh_hz;
            config.orientation = output.orientation;
            config.scale = output.scale;
            config.group_id = mg::DisplayConfigurationLogicalGroupId(0);
            enconutered_first = true;
            result.push_back(config);
        });

        return result;
    }

    void apply_internal(mg::DisplayConfiguration& conf, std::vector<OutputConfig> const& configs) const
    {
        bool has_had_primary = false;
        bool has_applied_any = false;
        conf.for_each_output([&](mg::UserDisplayConfigurationOutput const& output)
        {
            if (!output.connected || output.modes.empty())
            {
                output.used = false;
                output.power_mode = mir_power_mode_off;
                return;
            }

            auto const config_it = std::ranges::find_if(configs, [&](OutputConfig const& card)
            {
                return card.name == output.name;
            });

            if (config_it == configs.end())
            {
                output.used = false;
                output.power_mode = mir_power_mode_off;
                mir::log_info("Unused output with name and ID: %s, %d", output.name.c_str(), output.card_id.as_value());
                return;
            }

            mir::log_info("Applying output with name and ID: %s, %d", output.name.c_str(), output.card_id.as_value());
            auto const& card = *config_it;
            output.used = true;
            output.power_mode = mir_power_mode_on;
            output.orientation = mir_orientation_normal;

            bool primary = false;
            if (card.primary && !has_had_primary)
            {
                has_had_primary = true;
                primary = true;
            }

            output.custom_attribute["primary"] = primary ? "true" : "false";

            if (card.position)
                output.top_left = card.position.value();
            else
                output.top_left = geom::Point(0, 0);

            mir::log_info("Output position is (%d, %d)", output.top_left.x.as_int(), output.top_left.y.as_int());

            auto const& modes = output.modes;
            uint32_t const preferred_mode_index { select_mode_index(output.preferred_mode_index, modes) };
            output.current_mode_index = preferred_mode_index;

            if (card.size)
            {
                bool matched_mode = false;

                for (uint32_t i = 0; i < modes.size(); i++)
                {
                    const auto& [size, vrefresh_hz] = modes[i];
                    if (size != card.size.value())
                        continue;

                    if (card.refresh.has_value())
                    {
                        if (std::abs(card.refresh.value() - vrefresh_hz) < 1.0)
                        {
                            output.current_mode_index = i;
                            matched_mode = true;
                        }
                    }
                    else if (output.modes[output.current_mode_index].size != card.size.value()
                        || output.modes[output.current_mode_index].vrefresh_hz < card.refresh)
                    {
                        output.current_mode_index = i;
                        matched_mode = true;
                    }
                }

                if (!matched_mode)
                {
                    if (card.refresh.has_value())
                    {
                        mir::log_warning("Display config contains unmatched mode: '%dx%d@%2.1f'",
                            card.size.value().width.as_int(), card.size.value().height.as_int(), card.refresh.value());
                    }
                    else
                    {
                        mir::log_warning("Display config contains unmatched mode: '%dx%d'",
                            card.size.value().width.as_int(), card.size.value().height.as_int());
                    }
                }
            }

            output.scale = card.scale;
            output.orientation = card.orientation;
            output.logical_group_id = card.group_id;
            has_applied_any = true;
        });

        if (!has_applied_any)
        {
            mir::log_warning("No output configurations were applied, we will apply the default so that the system is usable");
            apply_default(conf);
        }
    }
};

miracle::DisplayConfig::DisplayConfig() :
    DisplayConfig(get_display_config_path())
{
}

miracle::DisplayConfig::DisplayConfig(std::string const& path) :
    self(std::make_shared<Self>(path))
{
}

void miracle::DisplayConfig::reload()
{
    if (self->has_config_file() && self->try_load_from_file())
        self->apply_configuration_manually();
}

void miracle::DisplayConfig::test(std::vector<OutputConfig> const& configs)
{
    self->test(configs);
}

void miracle::DisplayConfig::write()
{
    self->write();
}

void miracle::DisplayConfig::update(OutputConfig const& config)
{
    self->update(config);
}

std::vector<miracle::DisplayConfig::OutputConfig> miracle::DisplayConfig::get_configs()
{
    return self->get_configs();
}

miracle::OutputConfigDetailList miracle::DisplayConfig::configuration() const
{
    return self->configuration();
}

void miracle::DisplayConfig::operator()(mir::Server& server)
{
    auto constexpr config_file_name_option = "display-config-path";
    server.add_configuration_option(
        config_file_name_option,
        "File path to the display configuration file for miracle",
        self->path);

    server.wrap_display_configuration_policy([&](auto const&)
    {
        auto const server_opts = server.get_options();
        self->path = server_opts->get<std::string>(config_file_name_option);
        server.add_init_callback([self = self, &server]
        {
            self->display_configuration_controller = server.the_display_configuration_controller();
        });
        return self;
    });
}
