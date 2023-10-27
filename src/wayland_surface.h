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
#include "protocols/xdg_shell.h"
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
    /// output can be null, user needs to commit after
    void set_fullscreen(wl_output* output);
    void add_frame_callback(std::function<void()>&& func);

    auto app() const -> WaylandApp const* { return app_; }
    auto surface() const -> wl_surface* { return surface_; }
    auto configured_size() const -> mir::geometry::Size { return configured_size_; };

protected:
    /// Called when the compositor configures this shell surface
    virtual void configured() {};

private:
    static void handle_ping(void* data, struct wl_shell_surface* shell_surface, uint32_t serial);
    static void handle_configure(
        void* data,
        wl_shell_surface* shell_surface,
        uint32_t edges,
        int32_t width,
        int32_t height);

    static wl_shell_surface_listener const shell_surface_listener;
    static mir::geometry::Size const default_size;

    WaylandApp const* const app_;
    WaylandObject<wl_surface> const surface_;
    WaylandObject<wl_shell_surface> const shell_surface_;
    mir::geometry::Size configured_size_;
    int buffer_scale{1};
};
}


#endif // MIRAL_WAYLAND_SURFACE_H
