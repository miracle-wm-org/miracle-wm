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

#define MIR_LOG_COMPONENT "display_config"

#include "display_config.h"
#include "miracle/miracle-wm-config.h"

#include <filesystem>
#include <fstream>
#include <mir/graphics/display_configuration_policy.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <mir/shell/display_configuration_controller.h>
#include <yaml-cpp/yaml.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
auto select_mode_index(size_t mode_index, std::vector<mg::DisplayConfigurationMode> const& modes) -> size_t
{
    if (modes.empty())
        return std::numeric_limits<size_t>::max();

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
struct convert<miracle::DisplayConfig::Card>
{
    static Node encode(const miracle::DisplayConfig::Card& card)
    {
        Node node;
        node["enabled"] = card.enabled;
        node["name"] = card.name;
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

    static bool decode(const Node& node, miracle::DisplayConfig::Card& card)
    {
        card.name = node["name"].as<std::string>();
        card.enabled = node["enabled"].as<bool>(false);

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

template <>
struct convert<miracle::DisplayConfig::Layout>
{
    static Node encode(const miracle::DisplayConfig::Layout& layout)
    {
        Node node;
        node["name"] = layout.name;
        node["outputs"] = layout.cards;
        return node;
    }

    static bool decode(const Node& node, miracle::DisplayConfig::Layout& layout)
    {
        layout.name = node["name"].as<std::string>();
        layout.cards = node["outputs"].as<std::vector<miracle::DisplayConfig::Card>>();
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
    }

    void apply_to(mg::DisplayConfiguration& conf) override
    {
        auto const layout_it = std::ranges::find_if(available_layouts, [&](Layout const& layout)
        {
            return layout.name == active_layout;
        });

        if (layout_it == available_layouts.end())
        {
            mir::log_info("Invalid layout selected: %s. Selecting default", active_layout.c_str());
            apply_default(conf);
            return;
        }

        const auto [name, cards] = *layout_it;
        conf.for_each_output([&](mg::UserDisplayConfigurationOutput const& output)
        {
            if (!output.connected || output.modes.empty())
            {
                output.used = false;
                output.power_mode = mir_power_mode_off;
                return;
            }

            auto const card_it = std::ranges::find_if(cards, [&](Card const& card)
            {
                return card.name == output.name;
            });

            if (card_it == cards.end())
            {
                output.used = false;
                output.power_mode = mir_power_mode_off;
                mir::log_info("Unused output with name and ID: %s, %d", output.name.c_str(), output.card_id.as_value());
                return;
            }

            auto const& card = *card_it;
            output.used = true;
            output.power_mode = mir_power_mode_on;
            output.orientation = mir_orientation_normal;

            if (card.position)
                output.top_left = card.position.value();
            else
                output.top_left = geom::Point(0, 0);

            auto const& modes = output.modes;
            size_t const preferred_mode_index { select_mode_index(output.preferred_mode_index, modes) };
            output.current_mode_index = preferred_mode_index;

            if (card.size)
            {
                bool matched_mode = false;

                for (size_t i = 0; i < modes.size(); i++)
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
        });
    }

    void confirm(mg::DisplayConfiguration const& conf) override
    {
        cached = conf.clone();
    }

    void reload()
    {
        try
        {
            if (!std::filesystem::exists(path))
            {
                if (auto const dcc = display_configuration_controller.lock())
                {
                    auto const config = dcc->base_configuration();
                    auto const default_layout = to_layout(config);
                    YAML::Node node;
                    node.push_back(default_layout);

                    YAML::Node container;
                    container["layouts"] = node;
                    std::ofstream output_file(path);
                    output_file << container;
                }
            }

            mir::log_info("Loading display configuration from %s", path.c_str());
            YAML::Node const node = YAML::LoadFile(path);
            if (node["layouts"])
                available_layouts = node["layouts"].as<std::vector<Layout>>();
            else
                available_layouts.clear();

            if (!available_layouts.empty() && active_layout.empty())
                select_layout(available_layouts[0].name);

            mir::log_info("Display configuration loaded.");
        }
        catch (const std::exception& e)
        {
            mir::log_error("Exception during DisplayConfig reload: %s", e.what());
        }
        catch (...)
        {
            mir::log_error("Unknown exception during DisplayConfig reload");
        }
    }

    void select_layout(std::string const& name)
    {
        active_layout = "";
        for (auto const& layout : available_layouts)
        {
            if (layout.name == name)
            {
                active_layout = name;
                break;
            }
        }

        if (active_layout.empty())
        {
            mir::log_error("Invalid layout selected: %s", name.c_str());
            return;
        }

        if (auto const dcc = display_configuration_controller.lock())
        {
            auto const config = dcc->base_configuration();
            apply_to(*config);
            dcc->set_base_configuration(config);
        }
    }

    void write() const
    {
        YAML::Node node;
        for (const auto& layout : available_layouts)
        {
            node.push_back(layout);
        }

        YAML::Node container;
        container["layouts"] = node;
        std::ofstream output_file(path);
        output_file << container;
    }

    [[nodiscard]] std::optional<mg::DisplayConfiguration const*> configuration() const
    {
        if (!cached)
            return std::nullopt;

        return cached.get();
    }

    std::weak_ptr<mir::shell::DisplayConfigurationController> display_configuration_controller;
    std::string path;

private:
    std::vector<Layout> available_layouts;
    std::string active_layout;
    std::unique_ptr<mir::graphics::DisplayConfiguration> cached;
    mg::DisplayConfigurationLogicalGroupId const empty_group_id = mg::DisplayConfigurationLogicalGroupId(0);

    void apply_default(mg::DisplayConfiguration& conf) const
    {
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
            size_t const preferred_mode_index { select_mode_index(output.preferred_mode_index, output.modes) };
            output.current_mode_index = preferred_mode_index;
            output.logical_group_id = empty_group_id;
            output.orientation = mir_orientation_normal;
            position.x = geom::X { position.x.as_int() + output.extents().size.width.as_int() };
        });
    }

    static Layout to_layout(std::shared_ptr<mg::DisplayConfiguration> const& configuration)
    {
        Layout result;
        result.name = "default";

        geom::Point position(0, 0);
        configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output)
        {
            Card card;
            card.enabled = output.connected && !output.modes.empty();
            card.name = output.name;
            card.position = position;
            position.x = geom::X { position.x.as_int() + output.extents().size.width.as_int() };
            size_t const preferred_mode_index { select_mode_index(output.preferred_mode_index, output.modes) };
            auto const& mode = output.modes[preferred_mode_index];
            card.size = mode.size;
            card.refresh = mode.vrefresh_hz;
            card.orientation = output.orientation;
            card.scale = output.scale;
            card.group_id = mg::DisplayConfigurationLogicalGroupId(0);
            result.cards.push_back(card);
        });

        return result;
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

void miracle::DisplayConfig::reload() const
{
    self->reload();
}

void miracle::DisplayConfig::write() const
{
    self->write();
}

std::optional<mir::graphics::DisplayConfiguration const*> miracle::DisplayConfig::configuration() const
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
        return self;
    });

    server.add_init_callback([self = self, &server]
    {
        auto const server_opts = server.get_options();
        self->path = server_opts->get<std::string>(config_file_name_option);
        self->display_configuration_controller = server.the_display_configuration_controller();
        self->reload();
    });
}
