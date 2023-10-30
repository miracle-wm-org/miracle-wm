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

WaylandSurface::WaylandSurface(WaylandApp const* app)
    : app_{app},
      surface_{wl_compositor_create_surface(app->compositor()), wl_surface_destroy}
{
}

void WaylandSurface::attach_buffer(wl_buffer* buffer, int scale)
{
    float buffer_scale = 1.0;
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

void WaylandSurface::add_frame_callback(std::function<void()>&& func)
{
    WaylandCallback::create(wl_surface_frame(surface_), std::move(func));
}