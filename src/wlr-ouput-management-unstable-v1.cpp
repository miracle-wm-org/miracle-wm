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
#include "compositor_state.h"
#include "display_config.h"

#include <memory>
#include <mir/graphics/display_configuration.h>
#include <mir/log.h>
#include <utility>

using namespace miracle;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
double hz_to_mhz(double hz)
{
    return hz * 1000.0;
}

double mhz_to_hz(double mhz)
{
    return mhz / 1000.0;
}
}

namespace miracle
{
/// This object represents an output mode that will be broadcasted by the [WlrOutputHeadV1].
class WlrOutputModeV1 : public mir::wayland::OutputModeV1
{
public:
    WlrOutputModeV1(
        mir::wayland::OutputHeadV1& parent,
        geom::Size const& size,
        double refresh,
        bool preferred) :
        OutputModeV1(parent),
        size(size),
        refresh(refresh),
        preferred(preferred)
    {
    }

    void send_inital()
    {
        send_size_event(size.width.as_int(), size.height.as_int());
        send_refresh_event(static_cast<int32_t>(hz_to_mhz(refresh)));
        if (preferred)
            send_preferred_event();
    }

    ~WlrOutputModeV1() override
    {
        send_finished_event();
    }

    geom::Size const size;
    double const refresh;
    bool const preferred;
};

/// This object gets broadcasted by the [WlrOutputManagerV1] for each active output.
class WlrOutputHeadV1 : public mir::wayland::OutputHeadV1
{
public:
    WlrOutputHeadV1(
        miral::Output const& output,
        mir::wayland::OutputManagerV1 const& parent,
        mg::DisplayConfigurationOutput const& config);
    ~WlrOutputHeadV1() override;
    void send_initial();
    void update(miral::Output const& updated, mg::DisplayConfigurationOutput const& updated_config);
    void enable();
    void disable();
    std::string name() const;

    miral::Output output;
    mg::DisplayConfigurationOutput config;
    std::vector<std::shared_ptr<WlrOutputModeV1>> modes;

private:
    void release() override;
    bool is_defunct = false;
    bool is_enabled = false;
};

class WlrOutputManagerV1 : public mir::wayland::OutputManagerV1
{
public:
    explicit WlrOutputManagerV1(wl_resource* resource,
        std::shared_ptr<DisplayConfig> const& config,
        std::vector<miral::Output> const& outputs);
    ~WlrOutputManagerV1() override;
    WlrOutputHeadV1 const* head(wl_resource* resource) const;
    void advise_output_create(miral::Output const& output);
    void advise_output_update(miral::Output const& updated, miral::Output const& original);
    void advise_output_delete(miral::Output const& output);

    std::shared_ptr<DisplayConfig> const config;

private:
    void create_output_internal(miral::Output const& output);
    void create_configuration(struct wl_resource* id, uint32_t serial) override;
    void stop() override;

    std::vector<std::shared_ptr<WlrOutputHeadV1>> heads;
    uint32_t serial = 0;
};

class WlrOutputConfigurationHeadV1 : public mir::wayland::OutputConfigurationHeadV1
{
public:
    WlrOutputConfigurationHeadV1(struct wl_resource* id, WlrOutputHeadV1 const*);
    void set_mode(struct wl_resource* mode) override;
    void set_custom_mode(int32_t width, int32_t height, int32_t refresh) override;
    void set_position(int32_t x, int32_t y) override;
    void set_transform(int32_t transform) override;
    void set_scale(double scale) override;
    void set_adaptive_sync(uint32_t state) override;

    struct CustomMode
    {
        geom::Size size;
        double refresh;
    };

    WlrOutputHeadV1 const* head;
    std::optional<struct wl_resource*> mode;
    std::optional<CustomMode> custom_mode;
    std::optional<geom::Point> position;
    std::optional<int32_t> transform;
    std::optional<double> scale;
};

class WlrOutputConfigurationV1 : public mir::wayland::OutputConfigurationV1
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
    std::vector<std::shared_ptr<WlrOutputConfigurationHeadV1>> heads;

private:
    void discard_changes();
};

WlrOutputConfigurationV1::WlrOutputConfigurationV1(wl_resource* resource, WlrOutputManagerV1* manager) :
    mir::wayland::OutputConfigurationV1(resource, Version<4>()),
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

    heads.push_back(std::make_shared<WlrOutputConfigurationHeadV1>(id, head_object));
}

void WlrOutputConfigurationV1::disable_head(struct wl_resource* head)
{
    auto const it = std::find_if(this->heads.begin(), this->heads.end(), [&](std::shared_ptr<WlrOutputConfigurationHeadV1> const& other)
    {
        return other->head->resource == head;
    });
    if (it != heads.end())
    {
        heads.erase(it);
    }
    else
    {
        mir::log_error("Unable to disable_head because it cannot be found");
    }
}

void WlrOutputConfigurationV1::apply()
{
    std::vector<DisplayConfig::OutputConfig> new_outputs;
    auto constexpr apply_to_config = [&](
                                         std::shared_ptr<WlrOutputConfigurationHeadV1> const& head_config,
                                         DisplayConfig::OutputConfig& card)
    {
        card.name = head_config->head->output.name();
        if (head_config->position)
            card.position = *head_config->position;
        if (head_config->transform)
        {
            switch (*head_config->transform)
            {
            case 0:
                card.orientation = mir_orientation_normal;
                break;
            case 1:
                card.orientation = mir_orientation_left;
                break;
            case 2:
                card.orientation = mir_orientation_inverted;
                break;
            case 3:
                card.orientation = mir_orientation_right;
                break;
            case 4:
                card.orientation = mir_orientation_inverted;
                break;
            default:
                mir::log_warning("Flipped orientations are not supported");
                break;
            }
        }

        if (head_config->scale)
            card.scale = *head_config->scale;
        if (head_config->mode)
        {
            auto const& modes = head_config->head->modes;
            auto const& mode_resource = *head_config->mode;
            for (auto const& mode : modes)
            {
                if (mode->resource == mode_resource)
                {
                    card.size = mode->size;
                    card.refresh = mode->refresh;
                    break;
                }
            }
        }
    };

    auto const existing_configs = manager->config->get_configs();
    for (auto const& wlr_output_config_head : heads)
    {
        bool is_new = true;
        for (auto const& output_config : existing_configs)
        {
            if (output_config.name == wlr_output_config_head->head->name())
            {
                auto new_output_config = output_config;
                apply_to_config(wlr_output_config_head, new_output_config);
                new_outputs.push_back(new_output_config);
                is_new = false;
                break;
            }
        }

        if (is_new)
        {
            DisplayConfig::OutputConfig new_output_config;
            apply_to_config(wlr_output_config_head, new_output_config);
            new_outputs.push_back(new_output_config);
        }
    }

    for (auto const& output : new_outputs)
        manager->config->update(output);
    manager->config->write();
    manager->config->reload();
    send_succeeded_event();
}

void WlrOutputConfigurationV1::test()
{
}

void WlrOutputConfigurationV1::destroy()
{
    discard_changes();
}

void WlrOutputConfigurationV1::discard_changes()
{
    for (auto const& head : heads)
    {
        head->mode.reset();
        head->custom_mode.reset();
        head->position.reset();
        head->transform.reset();
        head->scale.reset();
    }
}

WlrOutputManagerV1::WlrOutputManagerV1(
    wl_resource* resource,
    std::shared_ptr<DisplayConfig> const& config,
    std::vector<miral::Output> const& outputs) :
    OutputManagerV1(resource, Version<4>()),
    config(config)
{
    // Advise the user on all of the existing heads. It is important we create
    // the heads first, send the "done" event, and then advise on the information
    // about the heads. This is the order that the spc wants this to happen.
    for (auto const& output : outputs)
        create_output_internal(output);
    send_done_event(serial++);

    for (auto const& head : heads)
        head->send_initial();
}

WlrOutputManagerV1::~WlrOutputManagerV1()
{
    send_finished_event();
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

void WlrOutputManagerV1::create_output_internal(miral::Output const& output)
{
    if (!config->configuration().has_value())
    {
        mir::log_error("Unable to create head because the display configuration is not set");
        return;
    }

    auto const display_config = config->configuration().value()->clone();
    mg::DisplayConfigurationOutput output_config;
    display_config->for_each_output([&](mg::DisplayConfigurationOutput const& config)
    {
        if (output.name() == config.name)
            output_config = config;
    });

    heads.push_back(std::make_shared<WlrOutputHeadV1>(output, *this, output_config));
    send_head_event(heads.back()->resource);
}

void WlrOutputManagerV1::advise_output_create(miral::Output const& output)
{
    create_output_internal(output);
    heads.back()->send_initial();
}

void WlrOutputManagerV1::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    for (auto const& head : heads)
    {
        if (head->output.id() == original.id())
        {
            auto const display_config = config->configuration().value()->clone();
            mg::DisplayConfigurationOutput output_config;
            display_config->for_each_output([&](mg::DisplayConfigurationOutput const& config)
            {
                if (updated.name() == config.name)
                    output_config = config;
            });

            head->update(updated, output_config);
            break;
        }
    }
    send_done_event(serial++);
}

void WlrOutputManagerV1::advise_output_delete(miral::Output const& output)
{
    auto const it = std::find_if(heads.begin(), heads.end(), [&](auto const& head)
    {
        return head->output.id() == output.id();
    });
    if (it != heads.end())
    {
        heads.erase(it);
        send_done_event(serial++);
    }
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
    mir::wayland::OutputManagerV1 const& parent,
    mg::DisplayConfigurationOutput const& config) :
    OutputHeadV1(parent),
    output { output },
    config { std::move(config) }
{
}

WlrOutputHeadV1::~WlrOutputHeadV1()
{
    if (is_defunct)
        return;

    send_finished_event();
}

void WlrOutputHeadV1::send_initial()
{
    if (is_defunct)
        return;

    // Warning: For many clients, it is very important that you inform them of the modes
    // and then the initial state of those modes first, especially before telling them
    // about the current mode
    for (size_t i = 0; i < config.modes.size(); ++i)
    {
        auto const& mode = config.modes[i];
        modes.push_back(std::make_shared<WlrOutputModeV1>(
            *this,
            mode.size,
            mode.vrefresh_hz,
            i == config.current_mode_index));
        send_mode_event(modes.back()->resource);
    }

    for (auto const& mode : modes)
        mode->send_inital();

    send_name_event(output.name());
    send_description_event("");
    send_physical_size_event(output.physical_size_mm().width, output.physical_size_mm().height);
    send_enabled_event(output.used());
    send_current_mode_event(modes[config.current_mode_index]->resource);
    send_position_event(output.extents().top_left.x.as_int(), output.extents().top_left.y.as_int());
    // TODO: Send the transform event via send_transform_event()
    send_scale_event(output.scale());
    // TODO: Send the make event via send_make_event()
    // TODO: Send the model event via send_model_event()
    // TODO: Send the serial_number event via send_serial_number_event()
    // TODO: Send the adapative_sync_event via send_adapative_sync_event
}

void WlrOutputHeadV1::update(miral::Output const& updated, mg::DisplayConfigurationOutput const& updated_config)
{
    if (is_defunct)
        return;

    if (output.name() != updated.name())
        send_name_event(updated.name());

    if (output.used() != updated.used())
        send_enabled_event(updated.used());

    if (config.current_mode_index != updated_config.current_mode_index)
        send_current_mode_event(modes[updated_config.current_mode_index]->resource);

    if (output.extents() != updated.extents())
        send_position_event(updated.extents().top_left.x.as_int(), updated.extents().top_left.y.as_int());

    if (output.scale() != updated.scale())
        send_scale_event(updated.scale());

    output = updated;
    config = updated_config;
}

void WlrOutputHeadV1::enable()
{
    is_enabled = true;
}

void WlrOutputHeadV1::disable()
{
    is_enabled = false;
}

std::string WlrOutputHeadV1::name() const
{
    return output.name();
}

void WlrOutputHeadV1::release()
{
    is_defunct = true;
}

}

WlrOutputManagementUnstableV1::WlrOutputManagementUnstableV1(
    wl_display* display,
    std::shared_ptr<DisplayConfig> const& config) :
    mir::wayland::OutputManagerV1::Global(display, Version<4>()),
    config(config)
{
}

void WlrOutputManagementUnstableV1::bind(wl_resource* new_zwlr_output_manager_v1)
{
    active_managers.push_back(new WlrOutputManagerV1(new_zwlr_output_manager_v1, config, outputs));
}

void WlrOutputManagementUnstableV1::output_created(miral::Output const& output)
{
    outputs.push_back(output);
    for (auto const& manager : active_managers)
        manager->advise_output_create(output);
}

void WlrOutputManagementUnstableV1::output_updated(miral::Output const& updated, miral::Output const& original)
{
    std::ranges::replace_if(outputs, [&](miral::Output const& output)
    {
        return output.is_same_output(original);
    }, updated);
    for (auto const& manager : active_managers)
        manager->advise_output_update(updated, original);
}

void WlrOutputManagementUnstableV1::output_deleted(miral::Output const& output)
{
    std::erase_if(outputs, [&](miral::Output const& other)
    {
        return other.is_same_output(output);
    });
    for (auto const& manager : active_managers)
        manager->advise_output_delete(output);
}

WlrOutputConfigurationHeadV1::WlrOutputConfigurationHeadV1(
    struct wl_resource* id, WlrOutputHeadV1 const* head) :
    OutputConfigurationHeadV1(id, Version<4>()),
    head { head }
{
}

void WlrOutputConfigurationHeadV1::set_mode(struct wl_resource* in)
{
    mode = in;
}

void WlrOutputConfigurationHeadV1::set_custom_mode(int32_t width, int32_t height, int32_t refresh)
{
    custom_mode = {
        geom::Size(width, height),
        mhz_to_hz(refresh)
    };
}

void WlrOutputConfigurationHeadV1::set_position(int32_t x, int32_t y)
{
    position = geom::Point(x, y);
}

void WlrOutputConfigurationHeadV1::set_transform(int32_t in)
{
    transform = in;
}

void WlrOutputConfigurationHeadV1::set_scale(double in)
{
    scale = in;
}

void WlrOutputConfigurationHeadV1::set_adaptive_sync(uint32_t state)
{
    mir::log_warning("set_adaptive_sync is not implemented");
}
