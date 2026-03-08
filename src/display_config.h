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

#ifndef MIRACLE_DISPLAY_CONFIG_H
#define MIRACLE_DISPLAY_CONFIG_H

#include "mir_version_manager.h"
#include <mir/fd.h>
#include <mir/geometry/point.h>
#include <mir/graphics/display_configuration.h>

namespace mir
{
class Server;
class MainLoop;
}

namespace miracle
{

/// This struct loosely mirrors [mir::graphics::DisplayConfigurationOutput], however,
/// it gets us around a tricky bug where that class could be deinitialized when the
/// program closes, causing us to segfault. This is most likely due to the fact that
/// the graphics platform modules were probably offloaded before we hit the [DisplayConfig]
/// deconstructor. Such is life.
///
/// At any rate, it is useful to have this struct in our control, so that is where we
/// will keep it for now.
struct OutputConfigDetails
{
    std::string name;
    mir::graphics::DisplayConfigurationCardId card_id;
    mir::geometry::Size size;
    mir::geometry::Point position;
    double scale;
    MirOrientation orientation;
    mir::graphics::DisplayConfigurationLogicalGroupId group_id;
    std::vector<mir::graphics::DisplayConfigurationMode> modes;
    std::optional<size_t> current_mode_index;
    bool used;
    MirPowerMode power_mode;
    MirPixelFormat current_format;
    bool is_primary;
    mir::graphics::DisplayInfo display_info;
};

typedef std::vector<OutputConfigDetails> OutputConfigDetailList;

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
        float scale = 1.f;
        mir::graphics::DisplayConfigurationLogicalGroupId group_id;
    };

    DisplayConfig();
    explicit DisplayConfig(std::string const& path);
    ~DisplayConfig();
    void reload();
    void test(std::vector<OutputConfig> const& configs);
    void write();
    void update(OutputConfig const& card);
    std::vector<OutputConfig> get_configs();
    [[nodiscard]] OutputConfigDetailList configuration() const;
    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> self;
    std::shared_ptr<mir::MainLoop> main_loop;
    mir::Fd inotify_fd;
    int file_watch = 0;

    void _watch(std::shared_ptr<mir::MainLoop> const& main_loop);
};

} // miracle

#endif // MIRACLE_DISPLAY_CONFIG_H
