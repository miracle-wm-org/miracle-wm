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
    int id = 0;
    mir::graphics::DisplayConfigurationCardId card_id;
    mir::geometry::Size physical_size_mm;
    mir::geometry::Point position;
    double scale = 1.0;
    MirOrientation orientation = mir_orientation_normal;
    mir::graphics::DisplayConfigurationLogicalGroupId group_id;
    std::vector<mir::graphics::DisplayConfigurationMode> modes;
    std::optional<size_t> current_mode_index;
    /// Whether a display is physically attached to this output. A disconnected output is gone,
    /// while an output that is connected but not [used] is simply switched off.
    bool connected = false;
    bool used = false;
    MirPowerMode power_mode = mir_power_mode_off;
    MirPixelFormat current_format = mir_pixel_format_invalid;
    bool is_primary = false;
    mir::graphics::DisplayInfo display_info;
};

typedef std::vector<OutputConfigDetails> OutputConfigDetailList;

/// Implemented by anyone who needs to know about the display configuration that is
/// actually in effect, including the outputs that are switched off. Note that
/// [miral::Output] only ever describes outputs that are enabled, so anything that
/// cares about disabled outputs must listen here instead.
class DisplayConfigListener
{
public:
    virtual ~DisplayConfigListener() = default;
    virtual void display_configuration_changed(OutputConfigDetailList const& configuration) = 0;
};

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
        MirOrientation orientation = mir_orientation_normal;
        float scale = 1.f;
        mir::graphics::DisplayConfigurationLogicalGroupId group_id;
    };

    DisplayConfig();
    explicit DisplayConfig(std::string const& path);
    ~DisplayConfig();
    void reload();
    void test(std::vector<OutputConfig> const& configs);
    void apply_to_config(mir::graphics::DisplayConfiguration& conf);
    void write();
    void update(OutputConfig const& card);
    std::vector<OutputConfig> get_configs();
    [[nodiscard]] OutputConfigDetailList configuration() const;
    /// Registers a listener that is notified whenever the display configuration changes.
    /// The listener is dropped as soon as it expires.
    void register_listener(std::weak_ptr<DisplayConfigListener> const& listener);
    void operator()(mir::Server& server);

private:
    class Self;
    class ConfigObserver;
    std::shared_ptr<Self> self;
    std::shared_ptr<ConfigObserver> observer;
    std::shared_ptr<mir::MainLoop> main_loop;
    mir::Fd inotify_fd;
    int file_watch = 0;

    void _watch(std::shared_ptr<mir::MainLoop> const& main_loop);
};

} // miracle

#endif // MIRACLE_DISPLAY_CONFIG_H
