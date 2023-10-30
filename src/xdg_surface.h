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

#ifndef MIRCOMPOSITOR_XDG_SURFACE_H
#define MIRCOMPOSITOR_XDG_SURFACE_H

#include "wayland_app.h"
#include "wayland_surface.h"
#include <mir/geometry/rectangle.h>

namespace miracle
{

struct XdgSurfaceListener
{
    std::function<void()> on_configured = []{};
};

class XdgSurface
{
public:
    XdgSurface(WaylandApp const* app, XdgSurfaceListener callbacks = {});
    void configure(zxdg_surface_v6* xdg_surface, uint32_t serial);
    void set_window_geometry(mir::geometry::Rectangle const& geoometry);
    WaylandSurface& surface() { return surface_; };
    WaylandApp const* app() { return app_; };
    mir::geometry::Size size() { return size_; };

private:
    WaylandApp const* const app_;
    WaylandSurface surface_;
    WaylandObject<zxdg_surface_v6> const xdg_surface_;
    WaylandObject<zxdg_toplevel_v6> const xdg_toplevel_;
    mir::geometry::Size size_;
    XdgSurfaceListener callbacks;

    static zxdg_surface_v6_listener const listener;
    static mir::geometry::Rectangle const default_geometry;
    static zxdg_toplevel_v6_listener const toplevel_listener;

    static void configure(void* data, zxdg_surface_v6* xdg_surface, uint32_t serial);
    static void configure_toplevel(void* data, zxdg_toplevel_v6* toplevel, int32_t width, int32_t height, struct wl_array* states);

};

}

#endif //MIRCOMPOSITOR_XDG_SURFACE_H
