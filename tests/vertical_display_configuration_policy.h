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

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
uint32_t select_mode_index(uint32_t mode_index, std::vector<mg::DisplayConfigurationMode> const& modes)
{
    if (modes.empty())
        return std::numeric_limits<uint32_t>::max();

    if (mode_index >= modes.size())
        return 0;

    return mode_index;
}
}

/// Each screen placed to the right of the previous one
namespace miracle
{
namespace test
{
    class VerticalDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
    {
    public:
        void apply_to(mg::DisplayConfiguration& conf) override
        {
            int max_y = 0;

            conf.for_each_output(
                [&](mg::UserDisplayConfigurationOutput& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf_output.used = true;
                    conf_output.top_left = geom::Point { 0, max_y };
                    uint32_t preferred_mode_index { select_mode_index(conf_output.preferred_mode_index, conf_output.modes) };
                    conf_output.current_mode_index = preferred_mode_index;
                    conf_output.power_mode = mir_power_mode_on;
                    conf_output.orientation = mir_orientation_normal;
                    max_y += conf_output.extents().size.height.as_int();
                }
                else
                {
                    conf_output.used = false;
                    conf_output.power_mode = mir_power_mode_off;
                }
            });
        }

        void confirm(mg::DisplayConfiguration const& conf) override
        {
        }
    };
}
}
