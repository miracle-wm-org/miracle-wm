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

#define MIR_LOG_COMPONENT "wlr-output-management-unstable-v1"

#include "wlr-ouput-management-unstable-v1.h"
#include "output.h"

#include <mir/log.h>
#include <cstdint>

namespace
{

class WlrOutputHeadV1;

class WlrOutputManagerV1 : public mir::wayland::ZwlrOutputManagerV1
{
public:
    explicit WlrOutputManagerV1(wl_resource* resource);
    std::shared_ptr<WlrOutputHeadV1> head(wl_resource* resource) const;

private:
    void create_configuration(struct wl_resource* id, uint32_t serial) override;
    void stop() override;
    std::vector<std::shared_ptr<WlrOutputHeadV1>> heads;
};

/// This object gets broadcasted by the [WlrOutputManagerV1] for each active output.
class WlrOutputHeadV1 : public mir::wayland::ZwlrOutputHeadV1
{
public:
    WlrOutputHeadV1(
        std::shared_ptr<miracle::Output> const& output,
        mir::wayland::ZwlrOutputManagerV1 const& parent);

    std::weak_ptr<miracle::Output> const output;
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
    WlrOutputConfigurationHeadV1(struct wl_resource* id, std::shared_ptr<WlrOutputHeadV1> const&);
    void set_mode(struct wl_resource* mode) override;
    void set_custom_mode(int32_t width, int32_t height, int32_t refresh) override;
    void set_position(int32_t x, int32_t y) override;
    void set_transform(int32_t transform) override;
    void set_scale(double scale) override;

private:
    std::shared_ptr<WlrOutputHeadV1> head;
};

WlrOutputConfigurationV1::WlrOutputConfigurationV1(wl_resource* resource, WlrOutputManagerV1* manager)
    : mir::wayland::ZwlrOutputConfigurationV1(resource, Version<2>()),
      manager{manager}
{}

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

WlrOutputManagerV1::WlrOutputManagerV1(wl_resource* resource)
    : mir::wayland::ZwlrOutputManagerV1(resource, Version<2>())
{}

std::shared_ptr<WlrOutputHeadV1> WlrOutputManagerV1::head(wl_resource* resource) const
{
    for (auto const& head: heads)
    {
        if (head->resource == resource)
            return head;
    }

    mir::fatal_error("Unable to find head with resource: %d", resource);
    return nullptr;
}

void WlrOutputManagerV1::create_configuration(struct wl_resource* id, uint32_t serial)
{
    new WlrOutputConfigurationV1(id, this);
}

void WlrOutputManagerV1::stop()
{
}

WlrOutputHeadV1::WlrOutputHeadV1(
    std::shared_ptr<miracle::Output> const& output,
    mir::wayland::ZwlrOutputManagerV1 const& parent)
    : mir::wayland::ZwlrOutputHeadV1(parent), output{output}
{
    // TODO: Register a listener on the output
}

}

void miracle::WlrOutputManagementUnstableV1::bind(wl_resource* new_zwlr_output_manager_v1)
{
    new WlrOutputManagerV1(new_zwlr_output_manager_v1);
}

WlrOutputConfigurationHeadV1::WlrOutputConfigurationHeadV1(
    struct wl_resource* id, std::shared_ptr<WlrOutputHeadV1> const& head)
    : mir::wayland::ZwlrOutputConfigurationHeadV1(id, Version<2>()),
      head{head}
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