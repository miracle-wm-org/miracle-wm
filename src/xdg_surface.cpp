/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xdg_surface.h"

using namespace miracle;

zxdg_surface_v6_listener const XdgSurface::listener = {
    configure
};

zxdg_toplevel_v6_listener const XdgSurface::toplevel_listener = {
    configure_toplevel,
    [](void*, zxdg_toplevel_v6*){}
};

mir::geometry::Rectangle const XdgSurface::default_geometry = {
    mir::geometry::Point{0, 0},
    mir::geometry::Size{640, 480}
};

XdgSurface::XdgSurface(const miracle::WaylandApp *app, XdgSurfaceListener callbacks)
    : app_{app},
      surface_{WaylandSurface(app)},
      xdg_surface_{zxdg_shell_v6_get_xdg_surface(app->get_zxdg_shell_v6(), surface_.surface()), zxdg_surface_v6_destroy},
      xdg_toplevel_{zxdg_surface_v6_get_toplevel(xdg_surface_), zxdg_toplevel_v6_destroy},
      size_{default_geometry.size},
      callbacks{callbacks}
{
    set_window_geometry(default_geometry);
    zxdg_surface_v6_add_listener(xdg_surface_, &listener, this);
    zxdg_toplevel_v6_add_listener(xdg_toplevel_, &toplevel_listener, this);
}

void XdgSurface::configure(zxdg_surface_v6* xdg_surface, uint32_t serial)
{
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

void XdgSurface::set_window_geometry(mir::geometry::Rectangle const& geometry)
{
    size_ = geometry.size;
    zxdg_surface_v6_set_window_geometry(
        xdg_surface_,
        geometry.top_left.x.as_int(),
        geometry.top_left.y.as_int(),
        geometry.size.width.as_int(),
        geometry.size.height.as_int());
    surface_.commit();
}

void XdgSurface::configure(void* data, zxdg_surface_v6* xdg_surface, uint32_t serial)
{
    auto surface = static_cast<XdgSurface*>(data);
    zxdg_surface_v6_ack_configure(surface->xdg_surface_, serial);
    surface->callbacks.on_configured();
}

void XdgSurface::configure_toplevel(void* data, zxdg_toplevel_v6* toplevel, int32_t width, int32_t height, struct wl_array* states)
{
    // TODO: How many source of truth for sizes are there?
    //auto surface = static_cast<XdgSurface*>(data);
    //surface->size_ = {width, height};
}