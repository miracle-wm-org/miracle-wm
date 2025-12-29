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

#ifndef MIRACLE_WM_PARENT_BACKGROUND_INTERNAL_CLIENT_H
#define MIRACLE_WM_PARENT_BACKGROUND_INTERNAL_CLIENT_H

#include "shell_application_spawner.h"
#include <memory>
#include <wayland-client.h>

struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

namespace miracle
{

/// Internal Wayland client that creates a surface with a gradient background
/// and rounded corners (8px border radius by default).
///
/// This client is designed to be used as a visual background for parent containers
/// in the tiling window manager, providing a decorative element with smooth gradients.
class ParentBackgroundInternalClient : public ShellApplication
{
public:
    ParentBackgroundInternalClient();
    ~ParentBackgroundInternalClient() override;

    /// Called when the Wayland display connection is established.
    /// This is the main entry point that sets up the client and runs the event loop.
    /// \param display The Wayland display connection
    void operator()(wl_display* display);

    /// Called when the Mir session is established.
    /// \param session The Mir session (weak pointer)
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

    void stop() override;

    miral::Application application() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    // Wayland registry callbacks
    static void registry_handle_global(
        void* data,
        wl_registry* registry,
        uint32_t name,
        char const* interface,
        uint32_t version);

    static void registry_handle_global_remove(
        void* data,
        wl_registry* registry,
        uint32_t name);

    // XDG surface callbacks
    static void xdg_surface_configure(
        void* data,
        xdg_surface* xdg_surface,
        uint32_t serial);

    static void xdg_toplevel_configure(
        void* data,
        xdg_toplevel* xdg_toplevel,
        int32_t width,
        int32_t height,
        wl_array* states);

    static void xdg_toplevel_close(
        void* data,
        xdg_toplevel* xdg_toplevel);

    void create_shm_buffer(int width, int height);
    void draw_gradient();
    void apply_rounded_corners();
    void cleanup_wayland_objects();
};

} // namespace miracle

#endif // MIRACLE_WM_PARENT_BACKGROUND_INTERNAL_CLIENT_H
