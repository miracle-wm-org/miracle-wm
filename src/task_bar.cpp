/*
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

#include "task_bar.h"
#include "wayland_app.h"
#include "xdg_surface.h"
#include "wayland_shm.h"
#include <cstring>
#include <poll.h>
#include <sys/eventfd.h>

namespace miracle
{

class TaskBarSurface
{
public:
    TaskBarSurface(
        WaylandApp* app,
        WaylandOutput const* output)
        : surface(app, {
            .on_configured=[&]() { app->trigger_draw(); }
        }),
          output{output},
          shm{app->shm()}
    {
        app->roundtrip();
    }

    ~TaskBarSurface()
    {
        surface.app()->roundtrip();
    }

    auto buffer_size() -> mir::geometry::Size
    {
        return surface.size() * output->scale();
    }

    void draw()
    {
        auto const width = buffer_size().width.as_int();
        auto const height = buffer_size().height.as_int();

        if (width <= 0 || height <= 0)
            return;

        auto const stride = 4*width;

        auto const buffer = shm.get_buffer(buffer_size(), mir::geometry::Stride{stride});
        auto content_area = buffer->data();

        uint8_t const bottom_colour[] = { 0x20, 0x54, 0xe9 };   // Ubuntu orange
        uint8_t const top_colour[] =    { 0x33, 0x33, 0x33 };   // Cool grey

        char* row = static_cast<decltype(row)>(content_area);

        for (int j = 0; j < height; j++)
        {
            uint8_t pattern[4];

            for (auto i = 0; i != 3; ++i)
                pattern[i] = (j*bottom_colour[i] + (height-j)*top_colour[i])/height;
            pattern[3] = 0xff;

            uint32_t* pixel = (uint32_t*)row;
            for (int i = 0; i < width; i++)
                memcpy(pixel + i, pattern, sizeof pixel[i]);

            row += stride;
        }

        surface.surface().attach_buffer(buffer->use(), output->scale());
        surface.surface().commit();
    }

private:
    WaylandOutput const* const output;
    WaylandShm shm;
    XdgSurface surface;

};


class TaskBarClient : public WaylandApp
{
public:
    TaskBarClient(wl_display* display)
    {
        wayland_init(display);
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);
    }

    ~TaskBarClient()
    {}

    void draw() override
    {
        bool needs_flush = false;
        for (auto const& output : outputs)
        {
            output.second->draw();
            needs_flush = true;
        }
        if (needs_flush)
        {
            wl_display_flush(display());
        }
    }

protected:
    virtual void output_ready(WaylandOutput const* output) override
    {
        outputs.insert({
            output,
            std::make_shared<TaskBarSurface>(this, output)
        });
    }

    virtual void output_changed(WaylandOutput const* output) override
    {
        // TODO: Handle
    }

    virtual void output_gone(WaylandOutput const* output) override
    {
        // TODO: Handle
    }

private:
    std::map<WaylandOutput const*, std::shared_ptr<TaskBarSurface>> outputs;
};
}

miracle::TaskBar::TaskBar()
{

}

void miracle::TaskBar::operator()(wl_display *display)
{
    client = new TaskBarClient(display);
    client->trigger_draw();
    client->run(display);
    client = nullptr;
}

void miracle::TaskBar::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    std::lock_guard lock{mutex};
    weak_session = session;
}

void miracle::TaskBar::stop()
{
    if (client)
    {
        client->stop();
    }
}