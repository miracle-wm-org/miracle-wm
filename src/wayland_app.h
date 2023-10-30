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

#ifndef MIRAL_WAYLAND_APP_H
#define MIRAL_WAYLAND_APP_H

#include <wayland-client.h>
#include <memory>
#include <map>
#include <functional>
#include <cstdint>
#include <mir/fd.h>
#include "protocols/xdg_shell_unstable_v6.h"

namespace miracle
{
class WaylandApp;

template<typename T>
class WaylandObject
{
public:
    WaylandObject()
        : proxy{nullptr, [](auto){}}
    {
    }

    WaylandObject(T* proxy, void(*destroy)(T*))
        : proxy{proxy, destroy}
    {
    }

    operator T*() const
    {
        return proxy.get();
    }

private:
    std::unique_ptr<T, void(*)(T*)> proxy;
};

class WaylandCallback
{
public:
    /// Takes ownership of callback
    static void create(wl_callback* callback, std::function<void()>&& func);

private:
    WaylandCallback(WaylandObject<wl_callback> callback, std::function<void()>&& func)
        : callback{std::move(callback)},
          func{std::move(func)}
    {
    }

    static wl_callback_listener const callback_listener;

    WaylandObject<wl_callback> const callback;
    std::function<void()> const func;
};

class WaylandOutput
{
public:
    WaylandOutput(WaylandApp* app, wl_output* output);
    virtual ~WaylandOutput() = default;

    auto scale() const -> int { return scale_; }
    auto transform() const -> int { return transform_; }
    auto operator==(wl_output* other) const -> bool { return wl_ == other; }
    auto wl() const -> wl_output* { return wl_; }

private:
    static void handle_geometry(
        void* data,
        struct wl_output* wl_output,
        int32_t x,
        int32_t y,
        int32_t physical_width,
        int32_t physical_height,
        int32_t subpixel,
        const char *make,
        const char *model,
        int32_t transform);
    static void handle_mode(
        void *data,
        struct wl_output* wl_output,
        uint32_t flags,
        int32_t width,
        int32_t height,
        int32_t refresh);
    static void handle_done(void* data, struct wl_output* wl_output);
    static void handle_scale(void* data, struct wl_output* wl_output, int32_t factor);

    static wl_output_listener const output_listener;

    WaylandApp* const app;
    WaylandObject<wl_output> const wl_;
    bool has_initialized;
    bool state_dirty;
    int scale_;
    int32_t transform_;
};

class WaylandApp
{
public:
    WaylandApp();
    WaylandApp(wl_display* display);
    virtual ~WaylandApp() = default;

    /// Needs to be two-step initialized to virtual methods are called
    void wayland_init(wl_display* display);
    void roundtrip() const;

    auto display() const -> wl_display* { return display_.get(); };
    auto compositor() const -> wl_compositor* { return compositor_; };
    auto shm() const -> wl_shm* { return shm_; };
    auto seat() const -> wl_seat* { return seat_; };
    auto shell() const -> wl_shell* { return shell_; };
    auto get_zxdg_shell_v6() const -> zxdg_shell_v6* { return zxdg_shell_v6_; };
    void run(wl_display *display);
    void trigger_draw();
    virtual void draw() = 0;
    void stop();

protected:
    friend WaylandOutput;
    virtual void output_ready(WaylandOutput const*) {};
    virtual void output_changed(WaylandOutput const*) {};
    virtual void output_gone(WaylandOutput const*) {};

private:
    /// Doesn't disconnect the display, instead roundtrips it to make sure everything is cleaned up
    std::unique_ptr<wl_display, decltype(&wl_display_roundtrip)> display_;
    WaylandObject<wl_registry> registry_;

    WaylandObject<wl_compositor> compositor_;
    WaylandObject<wl_shm> shm_;
    WaylandObject<wl_seat> seat_;
    WaylandObject<wl_shell> shell_;
    WaylandObject<zxdg_shell_v6> zxdg_shell_v6_;

    static void handle_new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t version);
    static void handle_global_remove(void* data, struct wl_registry* registry, uint32_t name);

    static wl_registry_listener const registry_listener;

    /// Functions to call then drop when globals are removed
    std::map<uint32_t, std::function<void()>> global_remove_handlers;
    mir::Fd const shutdown_signal;
    mir::Fd const draw_signal;
};
}

#endif // MIRAL_WAYLAND_APP_H
