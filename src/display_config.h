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

#ifndef MIRACLE_DISPLAY_CONFIG_H
#define MIRACLE_DISPLAY_CONFIG_H

#include <mir/geometry/point.h>
#include <mir/graphics/display_configuration.h>

namespace mir
{
class Server;
}

namespace miracle
{

class DisplayConfig
{
public:
    struct OutputConfig
    {
        bool enabled = false;
        bool primary = false;
        std::string name;
        std::optional<mir::geometry::Point> position;
        std::optional<mir::geometry::Size> size;
        std::optional<double> refresh;
        MirOrientation orientation;
        double scale = 1.f;
        mir::graphics::DisplayConfigurationLogicalGroupId group_id;
    };

    DisplayConfig();
    explicit DisplayConfig(std::string const& path);
    void reload();
    void test(std::vector<OutputConfig> const& configs);
    void write();
    void update(OutputConfig const& card);
    std::vector<OutputConfig> get_configs();
    [[nodiscard]] std::optional<std::unique_ptr<mir::graphics::DisplayConfiguration>> configuration() const;
    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> self;
};

} // miracle

#endif // MIRACLE_DISPLAY_CONFIG_H
