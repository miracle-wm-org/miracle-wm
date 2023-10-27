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

#include "wayland_surface.h"

using namespace miracle;
namespace geom = mir::geometry;

// If building against newer Wayland protocol definitions we may miss trailing fields
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
wl_shell_surface_listener const WaylandSurface::shell_surface_listener {
    handle_ping,
    handle_configure,
    [](auto, auto){}  // popup_done
};
#pragma GCC diagnostic pop

mir::geometry::Size const WaylandSurface::default_size{640, 480};

WaylandSurface::WaylandSurface(WaylandApp const* app)
    : app_{app},
      surface_{wl_compositor_create_surface(app->compositor()), wl_surface_destroy},
      shell_surface_{wl_shell_get_shell_surface(app->shell(), surface_), wl_shell_surface_destroy},
      configured_size_{default_size}
{
    wl_shell_surface_add_listener(shell_surface_, &shell_surface_listener, this);
}

void WaylandSurface::attach_buffer(wl_buffer* buffer, int scale)
{
    if (buffer_scale != scale)
    {
        wl_surface_set_buffer_scale(surface_, scale);
        buffer_scale = scale;
    }
    wl_surface_attach(surface_, buffer, 0, 0);
}

void WaylandSurface::commit() const
{
    wl_surface_commit(surface_);
}

void WaylandSurface::set_fullscreen(wl_output* output)
{
    wl_shell_surface_set_fullscreen(
        shell_surface_,
        WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
        0,
        output);
}

void WaylandSurface::add_frame_callback(std::function<void()>&& func)
{
    WaylandCallback::create(wl_surface_frame(surface_), std::move(func));
}

void WaylandSurface::handle_ping(void* data, struct wl_shell_surface*, uint32_t serial)
{
    auto const self = static_cast<WaylandSurface*>(data);
    wl_shell_surface_pong(self->shell_surface_, serial);
}

void WaylandSurface::handle_configure(void* data, wl_shell_surface*, uint32_t /*edges*/, int32_t w, int32_t h)
{
    auto const self = static_cast<WaylandSurface*>(data);
    geom::Width width{w};
    geom::Height height{h};
    bool changed{false};

    if (width > geom::Width{} && width != self->configured_size_.width)
    {
        self->configured_size_.width = geom::Width{width};
        changed = true;
    }

    if (height > geom::Height{} && height != self->configured_size_.height)
    {
        self->configured_size_.height = geom::Height{height};
        changed = true;
    }

    if (changed)
    {
        self->configured();
    }
}
