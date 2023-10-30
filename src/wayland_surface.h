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

#ifndef MIRAL_WAYLAND_SURFACE_H
#define MIRAL_WAYLAND_SURFACE_H

#include "wayland_app.h"
#include "mir/geometry/size.h"
#include <functional>

namespace miracle
{
class WaylandSurface
{
public:
    WaylandSurface(WaylandApp const* app);
    virtual ~WaylandSurface() = default;

    void attach_buffer(wl_buffer* buffer, int scale);
    void commit() const;
    void add_frame_callback(std::function<void()>&& func);

    auto app() const -> WaylandApp const* { return app_; }
    auto surface() const -> wl_surface* { return surface_; }

private:

    WaylandApp const* const app_;
    WaylandObject<wl_surface> const surface_;
};
}


#endif // MIRAL_WAYLAND_SURFACE_H
