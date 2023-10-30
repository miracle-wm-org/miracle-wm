/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wayland_app.h"

#include <cstring>
#include <algorithm>
#include <poll.h>
#include <sys/eventfd.h>

using namespace miracle;

// If building against newer Wayland protocol definitions we may miss trailing fields
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

wl_callback_listener const WaylandCallback::callback_listener {
    []/* done */(void* data, wl_callback*, uint32_t)
    {
        auto const cb = static_cast<WaylandCallback*>(data);
        cb->func();
        delete cb;
    },
};

wl_output_listener const WaylandOutput::output_listener = {
    handle_geometry,
    handle_mode,
    handle_done,
    handle_scale,
};
#pragma GCC diagnostic pop

void WaylandCallback::create(wl_callback* callback, std::function<void()>&& func)
{
    auto const cb = new WaylandCallback({callback, wl_callback_destroy}, std::move(func));
    wl_callback_add_listener(callback, &callback_listener, cb);
    // Will destroy itself when called
}

namespace
{
void wl_output_release_if_valid(struct wl_output* wl_output)
{
    if (wl_output_get_version(wl_output) >= WL_OUTPUT_RELEASE_SINCE_VERSION)
        wl_output_release(wl_output);
}
}

WaylandOutput::WaylandOutput(WaylandApp* app, wl_output* output)
    : app{app},
      wl_{output, wl_output_release_if_valid},
      has_initialized{false},
      state_dirty{false},
      scale_{1},
      transform_{WL_OUTPUT_TRANSFORM_NORMAL}
{
    wl_output_add_listener(output, &output_listener, this);
}

void WaylandOutput::handle_geometry(
    void* data,
    struct wl_output* /*wl_output*/,
    int32_t /*x*/,
    int32_t /*y*/,
    int32_t /*physical_width*/,
    int32_t /*physical_height*/,
    int32_t /*subpixel*/,
    const char* /*make*/,
    const char* /*model*/,
    int32_t transform)
{
    auto self = static_cast<WaylandOutput*>(data);
    if (self->transform_ != transform)
    {
        self->transform_ = transform;
        self->state_dirty = true;
    }
}

void WaylandOutput::handle_mode(
    void* /*data*/,
    struct wl_output* /*wl_output*/,
    uint32_t /*flags*/,
    int32_t /*width*/,
    int32_t /*height*/,
    int32_t /*refresh*/)
{
}

void WaylandOutput::handle_done(void* data, struct wl_output* /*wl_output*/)
{
    auto self = static_cast<WaylandOutput*>(data);
    if (!self->has_initialized)
    {
        self->app->output_ready(self);
        self->state_dirty = false;
        self->has_initialized = true;
    }
    else if (self->state_dirty)
    {
        self->app->output_changed(self);
        self->state_dirty = false;
    }
}

void WaylandOutput::handle_scale(void* data, struct wl_output* /*wl_output*/, int32_t factor)
{
    auto self = static_cast<WaylandOutput*>(data);
    self->scale_ = factor;
}

wl_registry_listener const WaylandApp::registry_listener = {
    handle_new_global,
    handle_global_remove
};

WaylandApp::WaylandApp()
    : display_{nullptr, [](auto){ return 0; }},
      draw_signal{::eventfd(0, EFD_SEMAPHORE)},
      shutdown_signal{::eventfd(0, EFD_CLOEXEC)}
{
}

WaylandApp::WaylandApp(wl_display* display)
    : WaylandApp{}
{
    wayland_init(display);
}

void WaylandApp::wayland_init(wl_display* display)
{
    display_ = {display, wl_display_roundtrip};
    registry_ = {wl_display_get_registry(display), wl_registry_destroy};

    wl_registry_add_listener(registry_, &registry_listener, this);

    // Populate globals
    roundtrip();

    // Populate properties for outputs and other globals created in the first pass
    roundtrip();
}

void WaylandApp::roundtrip() const
{
    wl_display_roundtrip(display());
}

#include <iostream>
void WaylandApp::handle_new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    auto const self = static_cast<WaylandApp*>(data);
    std::cout << interface << std::endl;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor_ = {
            static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 3)),
            wl_compositor_destroy};
        self->global_remove_handlers[id] = [self](){ self->compositor_ = {}; };
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        self->shm_ = {
            static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, 1)),
            wl_shm_destroy};
        self->global_remove_handlers[id] = [self](){ self->shm_ = {}; };
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        self->seat_ = {
            static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, 4)),
            wl_seat_destroy};
        self->global_remove_handlers[id] = [self](){ self->seat_ = {}; };
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        self->shell_ = {
            static_cast<wl_shell*>(wl_registry_bind(registry, id, &wl_shell_interface, 1)),
            wl_shell_destroy};
        self->global_remove_handlers[id] = [self](){ self->shell_ = {}; };
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        auto const output_resource = static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, std::min(version, 3u)));
        // shared_ptr instead of unique_ptr only so it can be captured by the lambda
        auto output = std::make_shared<WaylandOutput>(self, output_resource);
        // global_remove_handlers will hold on to the output until it is removed
        self->global_remove_handlers[id] = [self, output = std::move(output)]()
            {
                self->output_gone(output.get());
            };
    }
    else if (strcmp(interface, "zxdg_shell_v6") == 0)
    {
        self->zxdg_shell_v6_ = {
            static_cast<zxdg_shell_v6*>(wl_registry_bind(registry, id, &zxdg_shell_v6_interface, 1 )),
            zxdg_shell_v6_destroy };
    }
}

void WaylandApp::handle_global_remove(void* data, struct wl_registry* /*registry*/, uint32_t id)
{
    auto const self = static_cast<WaylandApp*>(data);
    auto const handler = self->global_remove_handlers.find(id);
    if (handler != self->global_remove_handlers.end())
    {
        handler->second();
        self->global_remove_handlers.erase(handler);
    }
}

void WaylandApp::run(wl_display *display)
{
    enum FdIndices {
        display_fd = 0,
        shutdown,
        draw_,
        indices
    };

    pollfd fds[indices];
    fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0};
    fds[shutdown] = {shutdown_signal, POLLIN, 0};
    fds[draw_] = {draw_signal, POLLIN, 0};

    while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
    {
        while (wl_display_prepare_read(display) != 0)
        {
            if (wl_display_dispatch_pending(display) == -1)
            {
                return;
            }
        }

        if (poll(fds, indices, -1) == -1)
        {
            wl_display_cancel_read(display);
            // TODO: Error
            return;
        }

        if (fds[draw_].revents & (POLLIN | POLLERR))
        {
            eventfd_t foo;
            eventfd_read(draw_signal, &foo);
            draw();
        }

        if (fds[display_fd].revents & (POLLIN | POLLERR))
        {
            if (wl_display_read_events(display))
            {
                // TODO: Error
                return;
            }
        }
        else
        {
            wl_display_cancel_read(display);
        }
    }
}

void WaylandApp::trigger_draw()
{
    eventfd_write(draw_signal, 1);
}

void WaylandApp::stop()
{
    if (eventfd_write(shutdown_signal, 1) == -1)
    {
        // TODO: Error
        return;
    }
}