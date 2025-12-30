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

#include "output_listener.h"
#include "wlr-output-management-unstable-v1_wrapper.h"

#include <memory>
#include <miral/output.h>

namespace miracle
{
class WlrOutputManagerV1;
class DisplayConfig;

class WlrOutputManagementUnstableV1 : public mir::wayland::OutputManagerV1::Global, public OutputListener
{
public:
    WlrOutputManagementUnstableV1(wl_display*, std::shared_ptr<DisplayConfig> const& config);
    void output_created(miral::Output const& output) override;
    void output_updated(miral::Output const& updated, miral::Output const& original) override;
    void output_deleted(miral::Output const& output) override;

private:
    void bind(wl_resource* new_zwlr_output_manager_v1) override;

    std::vector<WlrOutputManagerV1*> active_managers;
    std::shared_ptr<DisplayConfig> config;
    std::vector<miral::Output> outputs;
};

}

#endif // MIRACLE_WM_WLR_OUPUT_MANAGEMENT_UNSTABLE_V1_H