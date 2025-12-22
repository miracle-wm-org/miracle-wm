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

#define MIR_LOG_COMPONENT "ParentBackgroundInternalClient"

#include "parent_background_internal_client.h"

#include <mir/log.h>
#include <mir/scene/session.h>

#include "xdg-shell-client-protocol.h"
#include <wayland-client.h>

#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace miracle
{

struct ParentBackgroundInternalClient::Impl
{
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    xdg_wm_base* xdg_wm_base = nullptr;

    wl_surface* surface = nullptr;
    xdg_surface* xdg_surface = nullptr;
    xdg_toplevel* xdg_toplevel = nullptr;

    wl_buffer* buffer = nullptr;
    void* shm_data = nullptr;
    size_t shm_size = 0;

    mir::geometry::Rectangle rectangle;
    int width = 800;
    int height = 600;

    float top_color[4] = { 0.2f, 0.3f, 0.5f, 1.0f }; // Bluish top
    float bottom_color[4] = { 0.1f, 0.15f, 0.25f, 1.0f }; // Darker blue bottom
    float border_radius = 8.0f;

    bool configured = false;
    std::weak_ptr<mir::scene::Session> session;

    Impl(mir::geometry::Rectangle const& rect) : rectangle(rect), width(rect.size.width.as_int()), height(rect.size.height.as_int()) {}
};

namespace
{
    // XDG WM Base listener
    void xdg_wm_base_ping(void* /*data*/, xdg_wm_base* xdg_wm_base, uint32_t serial)
    {
        xdg_wm_base_pong(xdg_wm_base, serial);
    }

    const struct xdg_wm_base_listener xdg_wm_base_listener = {
        xdg_wm_base_ping
    };

    // Shared memory pool creation
    int create_shm_file(size_t size)
    {
        int fd = memfd_create("miracle-wm-shm", MFD_CLOEXEC);
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

ParentBackgroundInternalClient::ParentBackgroundInternalClient(mir::geometry::Rectangle const& rectangle) :
    impl(std::make_unique<Impl>(rectangle))
{
    mir::log_info("ParentBackgroundInternalClient: Created with rectangle pos=(%d,%d) size=%dx%d",
        rectangle.top_left.x.as_int(), rectangle.top_left.y.as_int(),
        rectangle.size.width.as_int(), rectangle.size.height.as_int());
}

ParentBackgroundInternalClient::~ParentBackgroundInternalClient()
{
    if (impl->buffer)
        wl_buffer_destroy(impl->buffer);

    if (impl->shm_data)
        munmap(impl->shm_data, impl->shm_size);

    if (impl->xdg_toplevel)
        xdg_toplevel_destroy(impl->xdg_toplevel);

    if (impl->xdg_surface)
        xdg_surface_destroy(impl->xdg_surface);

    if (impl->surface)
        wl_surface_destroy(impl->surface);

    if (impl->xdg_wm_base)
        xdg_wm_base_destroy(impl->xdg_wm_base);

    if (impl->compositor)
        wl_compositor_destroy(impl->compositor);

    if (impl->shm)
        wl_shm_destroy(impl->shm);

    if (impl->registry)
        wl_registry_destroy(impl->registry);
}

void ParentBackgroundInternalClient::operator()(wl_display* display)
{
    mir::log_info("ParentBackgroundInternalClient: Starting internal client");

    impl->display = display;

    // Get the registry
    impl->registry = wl_display_get_registry(display);
    if (!impl->registry)
    {
        mir::log_error("Failed to get Wayland registry");
        return;
    }

    static const struct wl_registry_listener registry_listener = {
        registry_handle_global,
        registry_handle_global_remove
    };

    wl_registry_add_listener(impl->registry, &registry_listener, this);

    // Roundtrip to get globals
    wl_display_roundtrip(display);

    if (!impl->compositor || !impl->shm || !impl->xdg_wm_base)
    {
        mir::log_error("Required Wayland globals not available");
        return;
    }

    // Create surface
    impl->surface = wl_compositor_create_surface(impl->compositor);
    if (!impl->surface)
    {
        mir::log_error("Failed to create Wayland surface");
        return;
    }

    // Create XDG surface
    impl->xdg_surface = xdg_wm_base_get_xdg_surface(impl->xdg_wm_base, impl->surface);
    if (!impl->xdg_surface)
    {
        mir::log_error("Failed to create XDG surface");
        return;
    }

    static const struct xdg_surface_listener xdg_surface_listener = {
        xdg_surface_configure
    };

    xdg_surface_add_listener(impl->xdg_surface, &xdg_surface_listener, this);

    // Create XDG toplevel
    impl->xdg_toplevel = xdg_surface_get_toplevel(impl->xdg_surface);
    if (!impl->xdg_toplevel)
    {
        mir::log_error("Failed to create XDG toplevel");
        return;
    }

    static const struct xdg_toplevel_listener xdg_toplevel_listener = {
        xdg_toplevel_configure,
        xdg_toplevel_close
    };

    xdg_toplevel_add_listener(impl->xdg_toplevel, &xdg_toplevel_listener, this);

    xdg_toplevel_set_title(impl->xdg_toplevel, "Parent Background");
    xdg_toplevel_set_app_id(impl->xdg_toplevel, "miracle.parent_background");

    // Commit to trigger configure
    wl_surface_commit(impl->surface);

    // Run event loop
    while (!should_quit && wl_display_dispatch(display) != -1)
    {
        // Event loop continues until should_quit is set
    }

    mir::log_info("ParentBackgroundInternalClient: Exiting event loop");
}

void ParentBackgroundInternalClient::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    impl->session = session;
    mir::log_info("ParentBackgroundInternalClient: Session connected");
}

void ParentBackgroundInternalClient::set_gradient_colors(float top_color[4], float bottom_color[4])
{
    std::memcpy(impl->top_color, top_color, sizeof(float) * 4);
    std::memcpy(impl->bottom_color, bottom_color, sizeof(float) * 4);
}

void ParentBackgroundInternalClient::set_border_radius(float radius)
{
    impl->border_radius = radius;
}

void ParentBackgroundInternalClient::registry_handle_global(
    void* data,
    wl_registry* registry,
    uint32_t name,
    char const* interface,
    uint32_t version)
{
    auto* client = static_cast<ParentBackgroundInternalClient*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        client->impl->compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
        mir::log_info("Bound to wl_compositor");
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        client->impl->shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, std::min(version, 1u)));
        mir::log_info("Bound to wl_shm");
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        client->impl->xdg_wm_base = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 2u)));
        xdg_wm_base_add_listener(client->impl->xdg_wm_base, &xdg_wm_base_listener, client);
        mir::log_info("Bound to xdg_wm_base");
    }
}

void ParentBackgroundInternalClient::registry_handle_global_remove(
    void* /*data*/,
    wl_registry* /*registry*/,
    uint32_t /*name*/)
{
    // Handle global removal if needed
}

void ParentBackgroundInternalClient::xdg_surface_configure(
    void* data,
    xdg_surface* xdg_surface,
    uint32_t serial)
{
    auto* client = static_cast<ParentBackgroundInternalClient*>(data);

    xdg_surface_ack_configure(xdg_surface, serial);

    if (!client->impl->configured)
    {
        client->impl->configured = true;

        // Create and draw the buffer
        client->create_shm_buffer(client->impl->width, client->impl->height);
        client->draw_gradient();
        client->apply_rounded_corners();

        // Attach and commit
        wl_surface_attach(client->impl->surface, client->impl->buffer, 0, 0);
        wl_surface_commit(client->impl->surface);

        mir::log_info("Surface configured and drawn");
    }
}

void ParentBackgroundInternalClient::xdg_toplevel_configure(
    void* data,
    xdg_toplevel* /*xdg_toplevel*/,
    int32_t width,
    int32_t height,
    wl_array* /*states*/)
{
    auto* client = static_cast<ParentBackgroundInternalClient*>(data);

    if (width > 0 && height > 0)
    {
        client->impl->width = width;
        client->impl->height = height;
        mir::log_info("Toplevel configured to %dx%d", width, height);
    }
}

void ParentBackgroundInternalClient::xdg_toplevel_close(
    void* data,
    xdg_toplevel* /*xdg_toplevel*/)
{
    auto* client = static_cast<ParentBackgroundInternalClient*>(data);
    client->should_quit = true;
    mir::log_info("Toplevel close requested");
}

void ParentBackgroundInternalClient::create_shm_buffer(int width, int height)
{
    int stride = width * 4; // 4 bytes per pixel (ARGB8888)
    impl->shm_size = static_cast<size_t>(stride * height);

    int fd = create_shm_file(impl->shm_size);
    if (fd < 0)
    {
        mir::log_error("Failed to create shared memory file");
        return;
    }

    impl->shm_data = mmap(nullptr, impl->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (impl->shm_data == MAP_FAILED)
    {
        mir::log_error("mmap failed");
        close(fd);
        return;
    }

    wl_shm_pool* pool = wl_shm_create_pool(impl->shm, fd, static_cast<int32_t>(impl->shm_size));
    impl->buffer = wl_shm_pool_create_buffer(
        pool,
        0,
        width,
        height,
        stride,
        WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(pool);
    close(fd);

    mir::log_info("Created shared memory buffer %dx%d", width, height);
}

void ParentBackgroundInternalClient::draw_gradient()
{
    if (!impl->shm_data)
        return;

    auto* pixels = static_cast<uint32_t*>(impl->shm_data);

    for (int y = 0; y < impl->height; ++y)
    {
        // Calculate interpolation factor (0.0 at top, 1.0 at bottom)
        float t = static_cast<float>(y) / static_cast<float>(impl->height - 1);

        // Interpolate colors
        float r = impl->top_color[0] * (1.0f - t) + impl->bottom_color[0] * t;
        float g = impl->top_color[1] * (1.0f - t) + impl->bottom_color[1] * t;
        float b = impl->top_color[2] * (1.0f - t) + impl->bottom_color[2] * t;
        float a = impl->top_color[3] * (1.0f - t) + impl->bottom_color[3] * t;

        // Convert to ARGB8888
        uint32_t pixel = (static_cast<uint32_t>(a * 255) << 24) | (static_cast<uint32_t>(r * 255) << 16) | (static_cast<uint32_t>(g * 255) << 8) | (static_cast<uint32_t>(b * 255));

        for (int x = 0; x < impl->width; ++x)
        {
            pixels[y * impl->width + x] = pixel;
        }
    }
}

void ParentBackgroundInternalClient::apply_rounded_corners()
{
    if (!impl->shm_data || impl->border_radius <= 0.0f)
        return;

    auto* pixels = static_cast<uint32_t*>(impl->shm_data);
    float radius = impl->border_radius;

    // Apply rounded corners to all four corners
    auto apply_corner = [&](int cx, int cy, int x_start, int y_start, int x_dir, int y_dir)
    {
        int radius_ceil = static_cast<int>(std::ceil(radius));

        for (int dy = 0; dy < radius_ceil; ++dy)
        {
            for (int dx = 0; dx < radius_ceil; ++dx)
            {
                int x = x_start + dx * x_dir;
                int y = y_start + dy * y_dir;

                if (x < 0 || x >= impl->width || y < 0 || y >= impl->height)
                    continue;

                // Calculate distance from corner center
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

                // If outside the radius, make transparent
                if (dist > radius)
                {
                    pixels[y * impl->width + x] = 0x00000000; // Fully transparent
                }
                else if (dist > radius - 1.0f)
                {
                    // Anti-aliasing for smooth edges
                    float alpha = radius - dist;
                    uint32_t current = pixels[y * impl->width + x];
                    uint32_t current_alpha = (current >> 24) & 0xFF;
                    uint32_t new_alpha = static_cast<uint32_t>(current_alpha * alpha);
                    pixels[y * impl->width + x] = (current & 0x00FFFFFF) | (new_alpha << 24);
                }
            }
        }
    };

    // Top-left corner
    apply_corner(0, 0, 0, 0, 1, 1);

    // Top-right corner
    apply_corner(impl->width - 1, 0, impl->width - 1, 0, -1, 1);

    // Bottom-left corner
    apply_corner(0, impl->height - 1, 0, impl->height - 1, 1, -1);

    // Bottom-right corner
    apply_corner(impl->width - 1, impl->height - 1, impl->width - 1, impl->height - 1, -1, -1);
}

} // namespace miracle
