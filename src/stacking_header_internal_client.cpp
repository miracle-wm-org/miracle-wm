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

#define MIR_LOG_COMPONENT "StackingHeaderInternalClient"

#include "stacking_header_internal_client.h"
#include "xdg-shell-client-protocol.h"

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <mir/log.h>
#include <mir/scene/session.h>

#include <wayland-client.h>

#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <unistd.h>

namespace miracle
{

TabState::TabState()
{
    update_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (update_fd < 0)
        mir::log_error("TabState: failed to create eventfd");
}

TabState::~TabState()
{
    if (update_fd >= 0)
        close(update_fd);
}

struct StackingHeaderInternalClient::Impl
{
    std::mutex mutex;
    std::shared_ptr<TabState> tab_state;

    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    xdg_wm_base* xdg_wm_base_obj = nullptr;

    wl_surface* surface = nullptr;
    xdg_surface* xdg_surface_obj = nullptr;
    xdg_toplevel* xdg_toplevel_obj = nullptr;

    wl_buffer* buffer = nullptr;
    void* shm_data = nullptr;
    size_t shm_size = 0;

    int width = 800;
    int height = 30;

    bool configured = false;
    std::weak_ptr<mir::scene::Session> session;
    int quit_eventfd = -1;

    explicit Impl(std::shared_ptr<TabState> ts) :
        tab_state(std::move(ts))
    {
        quit_eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (quit_eventfd < 0)
            mir::log_error("StackingHeaderInternalClient: failed to create quit eventfd");
    }

    ~Impl()
    {
        if (quit_eventfd >= 0)
            close(quit_eventfd);
    }
};

namespace
{
    void xdg_wm_base_ping(void* /*data*/, xdg_wm_base* base, uint32_t serial)
    {
        xdg_wm_base_pong(base, serial);
    }

    const struct xdg_wm_base_listener xdg_wm_base_listener_data = {
        xdg_wm_base_ping
    };

    int create_shm_file(size_t size)
    {
        int fd = memfd_create("miracle-wm-stacking-header", MFD_CLOEXEC);
        if (fd < 0)
        {
            mir::log_error("memfd_create failed");
            return -1;
        }
        if (ftruncate(fd, static_cast<off_t>(size)) < 0)
        {
            mir::log_error("ftruncate failed");
            close(fd);
            return -1;
        }
        return fd;
    }
}

StackingHeaderInternalClient::StackingHeaderInternalClient(std::shared_ptr<TabState> tab_state) :
    impl(std::make_unique<Impl>(std::move(tab_state)))
{
}

StackingHeaderInternalClient::~StackingHeaderInternalClient()
{
    std::lock_guard lk(impl->mutex);
    if (impl->shm_data)
    {
        munmap(impl->shm_data, impl->shm_size);
        impl->shm_data = nullptr;
    }
}

void StackingHeaderInternalClient::operator()(wl_display* display)
{
    mir::log_info("StackingHeaderInternalClient: starting");

    {
        std::lock_guard lk(impl->mutex);
        impl->display = display;
        impl->registry = wl_display_get_registry(display);
        if (!impl->registry)
        {
            mir::log_error("StackingHeaderInternalClient: failed to get registry");
            return;
        }
    }

    static const struct wl_registry_listener registry_listener = {
        registry_handle_global,
        registry_handle_global_remove
    };
    wl_registry_add_listener(impl->registry, &registry_listener, this);
    wl_display_roundtrip(display);

    {
        std::lock_guard lk(impl->mutex);
        if (!impl->compositor || !impl->shm || !impl->xdg_wm_base_obj)
        {
            mir::log_error("StackingHeaderInternalClient: required globals unavailable");
            return;
        }

        impl->surface = wl_compositor_create_surface(impl->compositor);
        if (!impl->surface)
        {
            mir::log_error("StackingHeaderInternalClient: failed to create surface");
            return;
        }

        impl->xdg_surface_obj = xdg_wm_base_get_xdg_surface(impl->xdg_wm_base_obj, impl->surface);
        if (!impl->xdg_surface_obj)
        {
            mir::log_error("StackingHeaderInternalClient: failed to create xdg_surface");
            return;
        }
    }

    static const struct xdg_surface_listener xdg_surface_listener_data = {
        xdg_surface_configure
    };
    xdg_surface_add_listener(impl->xdg_surface_obj, &xdg_surface_listener_data, this);

    {
        std::lock_guard lk(impl->mutex);
        impl->xdg_toplevel_obj = xdg_surface_get_toplevel(impl->xdg_surface_obj);
        if (!impl->xdg_toplevel_obj)
        {
            mir::log_error("StackingHeaderInternalClient: failed to create toplevel");
            return;
        }
    }

    static const struct xdg_toplevel_listener xdg_toplevel_listener_data = {
        xdg_toplevel_configure,
        xdg_toplevel_close
    };
    xdg_toplevel_add_listener(impl->xdg_toplevel_obj, &xdg_toplevel_listener_data, this);

    {
        std::lock_guard lk(impl->mutex);
        xdg_toplevel_set_title(impl->xdg_toplevel_obj, "Stacking Header");
        xdg_toplevel_set_app_id(impl->xdg_toplevel_obj, "miracle.stacking_header");
        wl_surface_commit(impl->surface);
    }

    enum FdIndices
    {
        display_fd = 0,
        shutdown_fd,
        update_fd_idx,
        indices
    };

    pollfd fds[indices];
    fds[display_fd] = { wl_display_get_fd(display), POLLIN, 0 };
    fds[shutdown_fd] = { impl->quit_eventfd, POLLIN, 0 };
    fds[update_fd_idx] = { impl->tab_state->update_fd, POLLIN, 0 };

    while (!(fds[shutdown_fd].revents & (POLLIN | POLLERR)))
    {
        while (wl_display_prepare_read(display) != 0)
        {
            if (wl_display_dispatch_pending(display) == -1)
                mir::log_error("StackingHeaderInternalClient: dispatch_pending failed");
        }

        wl_display_flush(display);

        if (poll(fds, indices, -1) == -1)
            mir::log_error("StackingHeaderInternalClient: poll failed");

        if (fds[display_fd].revents & (POLLIN | POLLERR))
        {
            if (wl_display_read_events(display))
                mir::log_error("StackingHeaderInternalClient: read_events failed");
        }
        else
        {
            wl_display_cancel_read(display);
        }

        if (fds[update_fd_idx].revents & POLLIN)
        {
            uint64_t val = 0;
            read(impl->tab_state->update_fd, &val, sizeof(val));

            std::lock_guard lk(impl->mutex);
            if (impl->configured && impl->surface && impl->buffer)
            {
                draw_header();
                wl_surface_attach(impl->surface, impl->buffer, 0, 0);
                wl_surface_damage_buffer(impl->surface, 0, 0, impl->width, impl->height);
                wl_surface_commit(impl->surface);
                wl_display_flush(display);
            }
        }
    }

    mir::log_info("StackingHeaderInternalClient: event loop exiting");
}

void StackingHeaderInternalClient::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    std::lock_guard lk(impl->mutex);
    impl->session = session;
}

void StackingHeaderInternalClient::stop()
{
    cleanup_wayland_objects();
    if (impl->quit_eventfd >= 0)
    {
        uint64_t value = 1;
        if (write(impl->quit_eventfd, &value, sizeof(value)) != sizeof(value))
            mir::log_error("StackingHeaderInternalClient: failed to write quit_eventfd");
    }
}

void StackingHeaderInternalClient::cleanup_wayland_objects()
{
    std::lock_guard lk(impl->mutex);

    if (impl->buffer)
    {
        wl_buffer_destroy(impl->buffer);
        impl->buffer = nullptr;
    }
    if (impl->xdg_toplevel_obj)
    {
        xdg_toplevel_destroy(impl->xdg_toplevel_obj);
        impl->xdg_toplevel_obj = nullptr;
    }
    if (impl->xdg_surface_obj)
    {
        xdg_surface_destroy(impl->xdg_surface_obj);
        impl->xdg_surface_obj = nullptr;
    }
    if (impl->surface)
    {
        wl_surface_destroy(impl->surface);
        impl->surface = nullptr;
    }
    if (impl->xdg_wm_base_obj)
    {
        xdg_wm_base_destroy(impl->xdg_wm_base_obj);
        impl->xdg_wm_base_obj = nullptr;
    }
    if (impl->compositor)
    {
        wl_compositor_destroy(impl->compositor);
        impl->compositor = nullptr;
    }
    if (impl->shm)
    {
        wl_shm_destroy(impl->shm);
        impl->shm = nullptr;
    }
    if (impl->registry)
    {
        wl_registry_destroy(impl->registry);
        impl->registry = nullptr;
    }
    if (impl->display)
        wl_display_flush(impl->display);
}

miral::Application StackingHeaderInternalClient::application()
{
    std::lock_guard lk(impl->mutex);
    return impl->session.lock();
}

void StackingHeaderInternalClient::registry_handle_global(
    void* data,
    wl_registry* registry,
    uint32_t name,
    char const* interface,
    uint32_t version)
{
    auto* client = static_cast<StackingHeaderInternalClient*>(data);
    std::lock_guard lk(client->impl->mutex);

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        client->impl->compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        client->impl->shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, std::min(version, 1u)));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        client->impl->xdg_wm_base_obj = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 2u)));
        xdg_wm_base_add_listener(client->impl->xdg_wm_base_obj, &xdg_wm_base_listener_data, client);
    }
}

void StackingHeaderInternalClient::registry_handle_global_remove(
    void* /*data*/,
    wl_registry* /*registry*/,
    uint32_t /*name*/)
{
}

void StackingHeaderInternalClient::xdg_surface_configure(
    void* data,
    xdg_surface* surface,
    uint32_t serial)
{
    auto* client = static_cast<StackingHeaderInternalClient*>(data);
    xdg_surface_ack_configure(surface, serial);

    std::lock_guard lk(client->impl->mutex);
    if (!client->impl->configured)
    {
        client->impl->configured = true;
        client->create_shm_buffer(client->impl->width, client->impl->height);
        client->draw_header();
        wl_surface_attach(client->impl->surface, client->impl->buffer, 0, 0);
        wl_surface_commit(client->impl->surface);
    }
}

void StackingHeaderInternalClient::xdg_toplevel_configure(
    void* data,
    xdg_toplevel* /*toplevel*/,
    int32_t width,
    int32_t height,
    wl_array* /*states*/)
{
    auto* client = static_cast<StackingHeaderInternalClient*>(data);
    std::lock_guard lk(client->impl->mutex);

    if (width > 0 && height > 0)
    {
        bool size_changed = (client->impl->width != width || client->impl->height != height);
        client->impl->width = width;
        client->impl->height = height;

        if (client->impl->configured && size_changed)
        {
            if (client->impl->buffer)
            {
                wl_buffer_destroy(client->impl->buffer);
                client->impl->buffer = nullptr;
            }
            if (client->impl->shm_data)
            {
                munmap(client->impl->shm_data, client->impl->shm_size);
                client->impl->shm_data = nullptr;
            }
            client->create_shm_buffer(width, height);
            client->draw_header();
            wl_surface_attach(client->impl->surface, client->impl->buffer, 0, 0);
            wl_surface_damage_buffer(client->impl->surface, 0, 0, width, height);
            wl_surface_commit(client->impl->surface);
        }
    }
}

void StackingHeaderInternalClient::xdg_toplevel_close(
    void* /*data*/,
    xdg_toplevel* /*toplevel*/)
{
}

void StackingHeaderInternalClient::create_shm_buffer(int width, int height)
{
    int const stride = width * 4;
    impl->shm_size = static_cast<size_t>(stride * height);

    int fd = create_shm_file(impl->shm_size);
    if (fd < 0)
    {
        mir::log_error("StackingHeaderInternalClient: failed to create shm file");
        return;
    }

    impl->shm_data = mmap(nullptr, impl->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (impl->shm_data == MAP_FAILED)
    {
        mir::log_error("StackingHeaderInternalClient: mmap failed");
        close(fd);
        impl->shm_data = nullptr;
        return;
    }

    wl_shm_pool* pool = wl_shm_create_pool(impl->shm, fd, static_cast<int32_t>(impl->shm_size));
    impl->buffer = wl_shm_pool_create_buffer(
        pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
}

void StackingHeaderInternalClient::draw_header()
{
    if (!impl->shm_data)
        return;

    std::vector<std::string> names;
    int focused_idx = -1;
    {
        std::lock_guard lk(impl->tab_state->mutex);
        names = impl->tab_state->names;
        focused_idx = impl->tab_state->focused_index;
    }

    int const stride = impl->width * 4;
    cairo_surface_t* cs = cairo_image_surface_create_for_data(
        static_cast<unsigned char*>(impl->shm_data),
        CAIRO_FORMAT_ARGB32,
        impl->width,
        impl->height,
        stride);
    cairo_t* cr = cairo_create(cs);

    cairo_set_source_rgba(cr, 0.18, 0.18, 0.18, 1.0);
    cairo_paint(cr);

    int const n = static_cast<int>(names.size());
    if (n > 0)
    {
        int const tab_w = impl->width / n;
        for (int i = 0; i < n; i++)
        {
            double const x = static_cast<double>(i * tab_w);

            if (i == focused_idx)
                cairo_set_source_rgba(cr, 0.30, 0.30, 0.45, 1.0);
            else
                cairo_set_source_rgba(cr, 0.24, 0.24, 0.24, 1.0);

            cairo_rectangle(cr, x + 1.0, 1.0, static_cast<double>(tab_w) - 2.0, static_cast<double>(impl->height) - 2.0);
            cairo_fill(cr);

            PangoLayout* layout = pango_cairo_create_layout(cr);
            PangoFontDescription* desc = pango_font_description_from_string("Sans 10");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);
            pango_layout_set_text(layout, names[i].c_str(), -1);
            pango_layout_set_width(layout, (tab_w - 8) * PANGO_SCALE);
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
            pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

            int text_w = 0, text_h = 0;
            pango_layout_get_pixel_size(layout, &text_w, &text_h);

            cairo_move_to(cr, x + 4.0, (static_cast<double>(impl->height) - static_cast<double>(text_h)) / 2.0);
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
            pango_cairo_show_layout(cr, layout);
            g_object_unref(layout);
        }
    }

    cairo_destroy(cr);
    cairo_surface_destroy(cs);
}

} // namespace miracle
