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

#ifndef MIRACLE_WM_STACKING_HEADER_INTERNAL_CLIENT_H
#define MIRACLE_WM_STACKING_HEADER_INTERNAL_CLIENT_H

#include "shell_application_spawner.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <wayland-client.h>

struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

namespace miracle
{

/// Shared state between StackingHeaderPositioner (main thread) and
/// StackingHeaderInternalClient (client thread). The update_fd eventfd
/// is created in the constructor; writing 1 to it signals a redraw.
struct TabState
{
    std::mutex mutex;
    std::vector<std::string> names;
    int focused_index = -1;
    int width = 0;
    int update_fd = -1;

    TabState();
    ~TabState();
};

/// Internal Wayland client that draws a tab/header bar at the top of
/// stacked and tabbed layout containers. Uses Cairo + Pango for rendering.
class StackingHeaderInternalClient : public ShellApplication
{
public:
    explicit StackingHeaderInternalClient(std::shared_ptr<TabState> tab_state);
    ~StackingHeaderInternalClient() override;

    void operator()(wl_display* display);
    void operator()(std::weak_ptr<mir::scene::Session> const& session);

    void stop() override;
    miral::Application application() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

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
    void draw_header();
    void cleanup_wayland_objects();
};

} // namespace miracle

#endif // MIRACLE_WM_STACKING_HEADER_INTERNAL_CLIENT_H
