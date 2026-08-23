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

#define MIR_LOG_COMPONENT "wlr-output-management-unstable-v1"

#include "wlr-ouput-management-unstable-v1.h"
#include "compositor_state.h"
#include "display_config.h"

#include <algorithm>
#include <memory>
#include <mir/graphics/display_configuration.h>
#include <mir/log.h>
#include <mir/wayland/client.h>
#include <mir/wayland/weak.h>
#include <utility>

using namespace miracle;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

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

/// Note on ownership throughout this file: every [mir::wayland::Resource] is owned by its
/// underlying wl_resource, not by us. The generated wrapper installs a destroy handler that
/// does `delete static_cast<T*>(wl_resource_get_user_data(resource))`, and the `release`/`destroy`
/// request handlers call `wl_resource_destroy` immediately after invoking our override. Holding
/// one of these objects in an owning smart pointer therefore creates a second owner and leads to
/// a double free. We only ever keep [mir::wayland::Weak] handles to them.
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
        if (is_defunct)
            return;

        send_size_event(size.width.as_int(), size.height.as_int());
        send_refresh_event(static_cast<int32_t>(hz_to_mhz(refresh)));
        if (preferred)
            send_preferred_event();
    }

    /// Announce that this mode is no longer available. The mode becomes inert but stays alive
    /// until the client releases it. Safe to call more than once.
    void send_finished()
    {
        if (is_defunct || finished_sent || client->is_being_destroyed())
            return;

        finished_sent = true;
        send_finished_event();
    }

    ~WlrOutputModeV1() override
    {
        send_finished();
    }

    void release() override
    {
        // The client is done with this mode. The wrapper destroys us as soon as we return,
        // so all this flag does is stop the destructor from replying with a "finished" event.
        is_defunct = true;
    }

    geom::Size const size;
    double const refresh;
    bool const preferred;

private:
    bool is_defunct = false;
    bool finished_sent = false;
};

/// This object gets broadcasted by the [WlrOutputManagerV1] for each active output.
class WlrOutputHeadV1 : public mir::wayland::OutputHeadV1
{
public:
    WlrOutputHeadV1(
        miral::Output const& output,
        mir::wayland::OutputManagerV1 const& parent,
        OutputConfigDetails const& config);
    ~WlrOutputHeadV1() override;
    void send_initial();
    void update(miral::Output const& updated, OutputConfigDetails const& updated_config);
    /// Announce that this head is no longer available. The head becomes inert but stays alive
    /// until the client releases it. Safe to call more than once.
    void send_finished();
    std::string name() const;

    miral::Output output;
    OutputConfigDetails config;
    std::vector<mir::wayland::Weak<WlrOutputModeV1>> modes;

private:
    /// Finishes the modes we previously broadcasted and broadcasts the ones in [new_config].
    /// [modes] indexes into whatever we last sent, so this must run before any index derived
    /// from a new config is used.
    void rebuild_modes(OutputConfigDetails const& new_config);
    void send_current_mode(OutputConfigDetails const& from);
    void release() override;
    bool is_defunct = false;
    bool finished_sent = false;
};

class WlrOutputManagerV1 : public mir::wayland::OutputManagerV1
{
public:
    explicit WlrOutputManagerV1(wl_resource* resource,
        std::shared_ptr<DisplayConfig> const& config,
        std::vector<miral::Output> const& outputs);
    ~WlrOutputManagerV1() override;
    WlrOutputHeadV1* head(wl_resource* resource);
    void advise_output_create(miral::Output const& output);
    void advise_output_update(miral::Output const& updated, miral::Output const& original);
    void advise_output_delete(miral::Output const& output);

    std::shared_ptr<DisplayConfig> const config;

private:
    WlrOutputHeadV1* create_output_internal(miral::Output const& output);
    void create_configuration(struct wl_resource* id, uint32_t serial) override;
    void stop() override;
    /// Drops handles to heads that the client has released out from under us.
    void prune_heads();

    std::vector<mir::wayland::Weak<WlrOutputHeadV1>> heads;
    uint32_t serial = 0;
};

class WlrOutputConfigurationHeadV1 : public mir::wayland::OutputConfigurationHeadV1
{
public:
    WlrOutputConfigurationHeadV1(struct wl_resource* id, WlrOutputHeadV1*);
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

    mir::wayland::Weak<WlrOutputHeadV1> head;
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

    mir::wayland::Weak<WlrOutputManagerV1> manager;
    std::vector<mir::wayland::Weak<WlrOutputConfigurationHeadV1>> heads;

private:
    void discard_changes();
    std::vector<DisplayConfig::OutputConfig> create_configs() const;
};

WlrOutputConfigurationV1::WlrOutputConfigurationV1(wl_resource* resource, WlrOutputManagerV1* manager) :
    mir::wayland::OutputConfigurationV1(resource, Version<4>()),
    manager { manager }
{
}

void WlrOutputConfigurationV1::enable_head(struct wl_resource* id, struct wl_resource* head)
{
    if (!manager)
        return;
    auto head_object = manager.value().head(head);
    if (!head_object)
    {
        mir::log_error("Unable to enable_head because it cannot be found");
        return;
    }

    // The configuration head is owned by its wl_resource, so we only track a weak handle.
    heads.push_back(mw::make_weak(new WlrOutputConfigurationHeadV1(id, head_object)));
}

void WlrOutputConfigurationV1::disable_head(struct wl_resource* head)
{
    auto const it = std::find_if(this->heads.begin(), this->heads.end(), [&](mw::Weak<WlrOutputConfigurationHeadV1> const& other)
    {
        return other && other.value().head && other.value().head.value().resource == head;
    });
    if (it != heads.end())
    {
        // Dropping the handle does not destroy the object: [create_configs] treats a head with
        // no entry in [heads] as disabled.
        heads.erase(it);
    }
    else
    {
        mir::log_error("Unable to disable_head because it cannot be found");
    }
}

void WlrOutputConfigurationV1::apply()
{
    if (!manager)
    {
        send_cancelled_event();
        return;
    }

    /// Finally, we update all of the configs, write the config, and reload.
    auto& mgr = manager.value();
    for (auto const& output : create_configs())
        mgr.config->update(output);
    mgr.config->write();
    mgr.config->reload();
    send_succeeded_event();
}

std::vector<DisplayConfig::OutputConfig> WlrOutputConfigurationV1::create_configs() const
{
    if (!manager)
        return {};

    std::vector<DisplayConfig::OutputConfig> new_outputs;
    auto const apply_to_config = [&](
                                     WlrOutputConfigurationHeadV1 const& head_config,
                                     DisplayConfig::OutputConfig& card)
    {
        if (!head_config.head)
            return;
        auto& head = head_config.head.value();
        card.name = head.output.name();
        card.enabled = true;
        if (head_config.position)
            card.position = *head_config.position;
        if (head_config.transform)
        {
            switch (*head_config.transform)
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

        if (head_config.scale)
            card.scale = *head_config.scale;
        if (head_config.mode)
        {
            auto const& mode_resource = *head_config.mode;
            for (auto const& mode : head.modes)
            {
                if (mode && mode.value().resource == mode_resource)
                {
                    card.size = mode.value().size;
                    card.refresh = mode.value().refresh;
                    break;
                }
            }
        }

        if (head_config.custom_mode)
        {
            card.size = head_config.custom_mode->size;
            card.refresh = head_config.custom_mode->refresh;
        }
    };

    /// First, we iterate over all of the heads that we have. We either update the existing head,
    /// or we add a new head if the head wasn't previously configured.
    auto const existing_configs = manager.value().config->get_configs();
    for (auto const& wlr_output_config_head : heads)
    {
        if (!wlr_output_config_head || !wlr_output_config_head.value().head)
            continue;
        auto const& head_config = wlr_output_config_head.value();
        bool is_new = true;
        for (auto const& output_config : existing_configs)
        {
            if (output_config.name == head_config.head.value().name())
            {
                auto new_output_config = output_config;
                apply_to_config(head_config, new_output_config);
                new_outputs.push_back(new_output_config);
                is_new = false;
                break;
            }
        }

        if (is_new)
        {
            DisplayConfig::OutputConfig new_output_config;
            apply_to_config(head_config, new_output_config);
            new_outputs.push_back(new_output_config);
        }
    }

    /// Next, iterate over the existing configurations. If a head doesn't exist for the existing config,
    /// this means that the user disabled it. In that case, mark it as disabled and push it to the new list.
    for (auto const& output_config : existing_configs)
    {
        bool found = false;
        for (auto const& wlr_output_config_head : heads)
        {
            if (!wlr_output_config_head)
                continue;
            auto const& head_config = wlr_output_config_head.value();
            if (head_config.head && output_config.name == head_config.head.value().name())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            auto new_output_config = output_config;
            new_output_config.enabled = false;
            new_outputs.push_back(new_output_config);
        }
    }

    return new_outputs;
}

void WlrOutputConfigurationV1::test()
{
    if (!manager)
        return;
    manager.value().config->test(create_configs());
}

void WlrOutputConfigurationV1::destroy()
{
    discard_changes();
}

void WlrOutputConfigurationV1::discard_changes()
{
    for (auto const& head : heads)
    {
        if (!head)
            continue;

        head.value().mode.reset();
        head.value().custom_mode.reset();
        head.value().position.reset();
        head.value().transform.reset();
        head.value().scale.reset();
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
    {
        if (head)
            head.value().send_initial();
    }
}

WlrOutputManagerV1::~WlrOutputManagerV1()
{
    if (!client->is_being_destroyed())
        send_finished_event();
}

void WlrOutputManagerV1::prune_heads()
{
    std::erase_if(heads, [](mw::Weak<WlrOutputHeadV1> const& head)
    { return !head; });
}

WlrOutputHeadV1* WlrOutputManagerV1::head(wl_resource* resource)
{
    prune_heads();
    for (auto const& head : heads)
    {
        if (head && head.value().resource == resource)
            return &head.value();
    }

    mir::log_error("Unable to find head with resource: %lu", reinterpret_cast<std::uintptr_t>(resource));
    return nullptr;
}

WlrOutputHeadV1* WlrOutputManagerV1::create_output_internal(miral::Output const& output)
{
    OutputConfigDetails output_config;
    for (auto const& other_output_config : config->configuration())
    {
        if (other_output_config.name == output.name())
            output_config = other_output_config;
    }

    // The head is owned by its wl_resource, so we only track a weak handle.
    auto* const head = new WlrOutputHeadV1(output, *this, output_config);
    heads.push_back(mw::make_weak(head));
    send_head_event(head->resource);
    return head;
}

void WlrOutputManagerV1::advise_output_create(miral::Output const& output)
{
    prune_heads();
    auto* const head = create_output_internal(output);
    send_done_event(serial++);
    head->send_initial();
}

void WlrOutputManagerV1::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    prune_heads();
    for (auto const& head : heads)
    {
        if (!head)
            continue;

        if (head.value().output.id() == original.id())
        {
            OutputConfigDetails output_config;
            for (auto const& other_output_config : config->configuration())
            {
                if (other_output_config.name == updated.name())
                    output_config = other_output_config;
            }

            head.value().update(updated, output_config);
            break;
        }
    }
    send_done_event(serial++);
}

void WlrOutputManagerV1::advise_output_delete(miral::Output const& output)
{
    prune_heads();
    auto const it = std::find_if(heads.begin(), heads.end(), [&](mw::Weak<WlrOutputHeadV1> const& head)
    {
        return head && head.value().output.id() == output.id();
    });
    if (it != heads.end())
    {
        // The head is gone, but the object it not ours to destroy: we announce that it is
        // finished, stop tracking it, and let the client release it when it is ready. Deleting
        // it here would leave a live wl_resource with a null implementation, and the client's
        // reply to "finished" would then be dispatched through a null vtable.
        it->value().send_finished();
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
    OutputConfigDetails const& config) :
    OutputHeadV1(parent),
    output { output },
    config { config }
{
}

WlrOutputHeadV1::~WlrOutputHeadV1()
{
    send_finished();
}

void WlrOutputHeadV1::send_finished()
{
    if (is_defunct || finished_sent || client->is_being_destroyed())
        return;

    finished_sent = true;
    for (auto const& mode : modes)
    {
        if (mode)
            mode.value().send_finished();
    }
    modes.clear();
    send_finished_event();
}

void WlrOutputHeadV1::rebuild_modes(OutputConfigDetails const& new_config)
{
    for (auto const& mode : modes)
    {
        if (mode)
            mode.value().send_finished();
    }
    modes.clear();

    for (size_t i = 0; i < new_config.modes.size(); ++i)
    {
        auto const& mode = new_config.modes[i];
        // Each mode is owned by its wl_resource, so we only track a weak handle.
        auto* const created = new WlrOutputModeV1(
            *this,
            mode.size,
            mode.vrefresh_hz,
            i == new_config.current_mode_index);
        modes.push_back(mw::make_weak(created));
        send_mode_event(created->resource);
    }
}

void WlrOutputHeadV1::send_current_mode(OutputConfigDetails const& from)
{
    if (!from.current_mode_index)
        return;

    auto const index = *from.current_mode_index;
    if (index >= modes.size() || !modes[index])
    {
        mir::log_warning(
            "Current mode index %zu is out of range for head '%s', which has %zu modes",
            index, output.name().c_str(), modes.size());
        return;
    }

    send_current_mode_event(modes[index].value().resource);
}

void WlrOutputHeadV1::send_initial()
{
    if (is_defunct)
        return;

    // Warning: For many clients, it is very important that you inform them of the modes
    // and then the initial state of those modes first, especially before telling them
    // about the current mode
    rebuild_modes(config);

    for (auto const& mode : modes)
    {
        if (mode)
            mode.value().send_inital();
    }

    send_name_event(output.name());
    send_description_event("");
    send_physical_size_event(output.physical_size_mm().width, output.physical_size_mm().height);
    send_enabled_event(output.used());
    send_current_mode(config);
    send_position_event(output.extents().top_left.x.as_int(), output.extents().top_left.y.as_int());
    // TODO: Send the transform event via send_transform_event()
    send_scale_event(output.scale());
    // TODO: Send the make event via send_make_event()
    // TODO: Send the model event via send_model_event()
    // TODO: Send the serial_number event via send_serial_number_event()
    // TODO: Send the adapative_sync_event via send_adapative_sync_event
}

void WlrOutputHeadV1::update(miral::Output const& updated, OutputConfigDetails const& updated_config)
{
    if (is_defunct)
        return;

    if (output.name() != updated.name())
        send_name_event(updated.name());

    if (output.used() != updated.used())
        send_enabled_event(updated.used());

    // [modes] holds the mode list we last broadcasted. If the list itself changed we have to
    // re-broadcast it, otherwise [current_mode_index] would index into a stale vector.
    bool const modes_changed = config.modes != updated_config.modes;
    if (modes_changed)
    {
        rebuild_modes(updated_config);
        for (auto const& mode : modes)
        {
            if (mode)
                mode.value().send_inital();
        }
    }

    if (modes_changed || config.current_mode_index != updated_config.current_mode_index)
        send_current_mode(updated_config);

    if (output.extents() != updated.extents())
        send_position_event(updated.extents().top_left.x.as_int(), updated.extents().top_left.y.as_int());

    if (output.scale() != updated.scale())
        send_scale_event(updated.scale());

    output = updated;
    config = updated_config;
}

std::string WlrOutputHeadV1::name() const
{
    return output.name();
}

void WlrOutputHeadV1::release()
{
    // The client is done with this head. The wrapper destroys us as soon as we return, so all
    // this flag does is stop the destructor from replying with a "finished" event. Our manager
    // holds a weak handle, which it prunes the next time it looks at its heads.
    is_defunct = true;
}

}

WlrOutputManagementUnstableV1::WlrOutputManagementUnstableV1(
    miral::WaylandExtensions::Context const* context,
    std::shared_ptr<DisplayConfig> const& config) :
    mir::wayland::OutputManagerV1::Global(context->display(), Version<4>()),
    context { context },
    config(config)
{
}

void WlrOutputManagementUnstableV1::bind(wl_resource* new_zwlr_output_manager_v1)
{
    active_managers.push_back(mir::wayland::Weak<WlrOutputManagerV1> {
        new WlrOutputManagerV1(new_zwlr_output_manager_v1, config, outputs) });
}

// The output_* methods below are called on the window management thread, but everything they
// touch - wl_resource_post_event, and mir::wayland::Weak itself - is only safe on the Wayland
// thread. Hop over before doing any work.

void WlrOutputManagementUnstableV1::output_created(miral::Output const& output)
{
    context->run_on_wayland_mainloop([this, output]
    {
        std::erase_if(active_managers, [](auto const& m)
        { return !m; });
        outputs.push_back(output);
        for (auto const& manager : active_managers)
            manager.value().advise_output_create(output);
    });
}

void WlrOutputManagementUnstableV1::output_updated(miral::Output const& updated, miral::Output const& original)
{
    context->run_on_wayland_mainloop([this, updated, original]
    {
        std::erase_if(active_managers, [](auto const& m)
        { return !m; });
        std::ranges::replace_if(outputs, [&](miral::Output const& output)
        {
            return output.is_same_output(original);
        }, updated);
        for (auto const& manager : active_managers)
            manager.value().advise_output_update(updated, original);
    });
}

void WlrOutputManagementUnstableV1::output_deleted(miral::Output const& output)
{
    context->run_on_wayland_mainloop([this, output]
    {
        std::erase_if(active_managers, [](auto const& m)
        { return !m; });
        std::erase_if(outputs, [&](miral::Output const& other)
        {
            return other.is_same_output(output);
        });
        for (auto const& manager : active_managers)
            manager.value().advise_output_delete(output);
    });
}

WlrOutputConfigurationHeadV1::WlrOutputConfigurationHeadV1(
    struct wl_resource* id, WlrOutputHeadV1* head) :
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
