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

#include <memory>
#include <shared_mutex>
#define MIR_LOG_COMPONENT "wlr-output-management-unstable-v1"

#include "wlr-ouput-management-unstable-v1.h"
#include "compositor_state.h"

#include <mir/log.h>
#include <cstdint>

using namespace miracle;

namespace miracle
{

/// This object gets broadcasted by the [WlrOutputManagerV1] for each active output.
class WlrOutputHeadV1 : public mir::wayland::ZwlrOutputHeadV1
{
public:
    WlrOutputHeadV1(
    miral::Output const& output,
    mir::wayland::ZwlrOutputManagerV1 const& parent);

    miral::Output const& output;
};

class WlrOutputManagerV1 : public mir::wayland::ZwlrOutputManagerV1
{
public:
    explicit WlrOutputManagerV1(wl_resource* resource);
    WlrOutputHeadV1 const* head(wl_resource* resource) const;
    void advise_output_create(miral::Output const& output);
    void advise_output_update(miral::Output const& updated, miral::Output const& original);
    void advise_output_delete(miral::Output const& output);

private:
    void create_configuration(struct wl_resource* id, uint32_t serial) override;
    void stop() override;

    std::vector<std::unique_ptr<WlrOutputHeadV1>> heads;
};

/// This object represents an output mode that will be broadcasted by the [WlrOutputHeadV1].
class WlrOutputModeV1 : public mir::wayland::ZwlrOutputModeV1
{
};

class WlrOutputConfigurationV1 : public mir::wayland::ZwlrOutputConfigurationV1
{
public:
    WlrOutputConfigurationV1(wl_resource* resource, WlrOutputManagerV1* manager);

private:
    void enable_head(struct wl_resource* id, struct wl_resource* head) override;
    void disable_head(struct wl_resource* head) override;
    void apply() override;
    void test() override;
    void destroy() override;

    WlrOutputManagerV1* manager;
};

class WlrOutputConfigurationHeadV1 : public mir::wayland::ZwlrOutputConfigurationHeadV1
{
public:
    WlrOutputConfigurationHeadV1(struct wl_resource* id, WlrOutputHeadV1 const*);
    void set_mode(struct wl_resource* mode) override;
    void set_custom_mode(int32_t width, int32_t height, int32_t refresh) override;
    void set_position(int32_t x, int32_t y) override;
    void set_transform(int32_t transform) override;
    void set_scale(double scale) override;

private:
    WlrOutputHeadV1 const* head;
};

WlrOutputConfigurationV1::WlrOutputConfigurationV1(wl_resource* resource, WlrOutputManagerV1* manager) :
    mir::wayland::ZwlrOutputConfigurationV1(resource, Version<2>()),
    manager { manager }
{
}

void WlrOutputConfigurationV1::enable_head(struct wl_resource* id, struct wl_resource* head)
{
    auto head_object = manager->head(head);
    if (!head_object)
    {
        mir::log_error("Unable to enable_head because it cannot be found");
        return;
    }

    new WlrOutputConfigurationHeadV1(id, head_object);
}

void WlrOutputConfigurationV1::disable_head(struct wl_resource* head)
{
}

void WlrOutputConfigurationV1::apply()
{
}

void WlrOutputConfigurationV1::test()
{
}

void WlrOutputConfigurationV1::destroy()
{
}

WlrOutputManagerV1::WlrOutputManagerV1(wl_resource* resource) :
    mir::wayland::ZwlrOutputManagerV1(resource, Version<2>())
{
}

WlrOutputHeadV1 const* WlrOutputManagerV1::head(wl_resource* resource) const
{
    for (auto const& head : heads)
    {
        if (head->resource == resource)
            return head.get();
    }

    mir::log_error("Unable to find head with resource: %lu", reinterpret_cast<std::uintptr_t>(resource));
    return nullptr;
}

void WlrOutputManagerV1::advise_output_create(miral::Output const& output)
{
    heads.push_back(std::make_unique<WlrOutputHeadV1>(output, *this));
}

void WlrOutputManagerV1::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
}

void WlrOutputManagerV1::advise_output_delete(miral::Output const& output)
{
}

void WlrOutputManagerV1::create_configuration(struct wl_resource* id, uint32_t serial)
{
    new WlrOutputConfigurationV1(id, this);
}

void WlrOutputManagerV1::stop()
{
}

WlrOutputHeadV1::WlrOutputHeadV1(
    miral::Output const& output,
    mir::wayland::ZwlrOutputManagerV1 const& parent) :
    mir::wayland::ZwlrOutputHeadV1(parent),
    output { output }
{
    // TODO: Register a listener on the output
}

}

WlrOutputManagementUnstableV1::WlrOutputManagementUnstableV1(wl_display* display, CompositorState const& state) :
    mir::wayland::ZwlrOutputManagerV1::Global(display, Version<2>()),
    state{state}
{
}

void WlrOutputManagementUnstableV1::bind(wl_resource* new_zwlr_output_manager_v1)
{
    active_managers.push_back(new WlrOutputManagerV1(new_zwlr_output_manager_v1));
}

void WlrOutputManagementUnstableV1::output_created(miral::Output const& output)
{
    for (auto const& manager : active_managers)
        manager->advise_output_create(output);
}

void WlrOutputManagementUnstableV1::output_updated(miral::Output const& updated, miral::Output const& original)
{
    for (auto const& manager : active_managers)
        manager->advise_output_update(updated, original);
}

void WlrOutputManagementUnstableV1::output_deleted(miral::Output const& output)
{
    for (auto const& manager : active_managers)
        manager->advise_output_delete(output);
}

WlrOutputConfigurationHeadV1::WlrOutputConfigurationHeadV1(
    struct wl_resource* id, WlrOutputHeadV1 const* head) :
    mir::wayland::ZwlrOutputConfigurationHeadV1(id, Version<2>()),
    head { head }
{
}

void WlrOutputConfigurationHeadV1::set_mode(struct wl_resource* mode)
{
}

void WlrOutputConfigurationHeadV1::set_custom_mode(int32_t width, int32_t height, int32_t refresh)
{
}

void WlrOutputConfigurationHeadV1::set_position(int32_t x, int32_t y)
{
}

void WlrOutputConfigurationHeadV1::set_transform(int32_t transform)
{
}

void WlrOutputConfigurationHeadV1::set_scale(double scale)
{
}
