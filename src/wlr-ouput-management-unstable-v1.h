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

#ifndef MIRACLE_WM_WLR_OUPUT_MANAGEMENT_UNSTABLE_V1_H
#define MIRACLE_WM_WLR_OUPUT_MANAGEMENT_UNSTABLE_V1_H

#include "display_config.h"
#include "wlr-output-management-unstable-v1_wrapper.h"

#include <memory>
#include <mir/wayland/weak.h>
#include <miral/wayland_extensions.h>

namespace miracle
{
class WlrOutputManagerV1;

/// Note that the heads are driven by the display configuration rather than by
/// [miral::Output], because miral only reports outputs that are switched on while the
/// protocol requires that a switched off head remains advertised with "enabled: 0".
class WlrOutputManagementUnstableV1 : public mir::wayland::OutputManagerV1::Global, public DisplayConfigListener
{
public:
    WlrOutputManagementUnstableV1(
        miral::WaylandExtensions::Context const* context,
        std::shared_ptr<DisplayConfig> const& config);
    void display_configuration_changed(OutputConfigDetailList const& configuration) override;

private:
    void bind(wl_resource* new_zwlr_output_manager_v1) override;

    /// Used to marshal output changes from the window management thread onto the Wayland
    /// thread, which is the only thread on which Wayland objects may be touched.
    miral::WaylandExtensions::Context const* const context;
    std::vector<mir::wayland::Weak<WlrOutputManagerV1>> active_managers;
    std::shared_ptr<DisplayConfig> config;
    /// The connected outputs as of the last configuration change, switched on or not.
    OutputConfigDetailList outputs;
};

}

#endif // MIRACLE_WM_WLR_OUPUT_MANAGEMENT_UNSTABLE_V1_H