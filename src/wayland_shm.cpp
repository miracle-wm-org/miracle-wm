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

#include "wayland_shm.h"
#include "wayland_app.h"

#include <mir/fd.h>
#include <mir/fatal.h>
#include <boost/throw_exception.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <system_error>

using namespace miracle;
namespace geom = mir::geometry;

namespace
{
static wl_shm_pool* make_shm_pool(wl_shm* shm, int size, void **data)
{
    static auto (*open_shm_file)() -> mir::Fd = []
        {
            static char const* shm_dir;
            open_shm_file = []{ return mir::Fd{open(shm_dir, O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU)}; };

            // Wayland based toolkits typically use $XDG_RUNTIME_DIR to open shm pools
            // so we try that before "/dev/shm". But confined snaps can't access "/dev/shm"
            // so we try "/tmp" if both of the above fail.
            for (auto dir : {const_cast<const char*>(getenv("XDG_RUNTIME_DIR")), "/dev/shm", "/tmp" })
            {
                if (dir)
                {
                    shm_dir = dir;
                    auto fd = open_shm_file();
                    if (fd >= 0)
                        return fd;
                }
            }

            // Workaround for filesystems that don't support O_TMPFILE (E.g. phones based on 3.4 or 3.10
            open_shm_file = []
            {
                char template_filename[] = "/dev/shm/mir-buffer-XXXXXX";
                mir::Fd fd{mkostemp(template_filename, O_CLOEXEC)};
                if (fd != -1)
                {
                    if (unlink(template_filename) < 0)
                    {
                        return mir::Fd{};
                    }
                }
                return fd;
            };

            return open_shm_file();
        };

    auto fd = open_shm_file();

    if (fd < 0) {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to open shm buffer"}));
    }

    if (auto error = posix_fallocate(fd, 0, size))
    {
        BOOST_THROW_EXCEPTION((std::system_error{error, std::system_category(), "Failed to allocate shm buffer"}));
    }

    if ((*data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to mmap buffer"}));
    }

    return wl_shm_create_pool(shm, fd, size);
}
}

struct miracle::WaylandShmPool
{
public:
    WaylandShmPool(wl_shm_pool* shm_pool, void* data, size_t size)
        : shm_pool{shm_pool},
          data{data},
          size{size}
    {
    }

    ~WaylandShmPool()
    {
        munmap(data, size);
        wl_shm_pool_destroy(shm_pool);
    }

    wl_shm_pool* const shm_pool;
    void* const data;
    size_t const size;
};

wl_buffer_listener const WaylandShmBuffer::buffer_listener {handle_release};

WaylandShmBuffer::WaylandShmBuffer(
    std::shared_ptr<WaylandShmPool> pool,
    void* data,
    geom::Size size,
    geom::Stride stride,
    wl_buffer* buffer)
    : pool{pool},
      data_{data},
      size_{size},
      stride_{stride},
      buffer{buffer}
{
    wl_buffer_add_listener(buffer, &buffer_listener, this);
}

WaylandShmBuffer::~WaylandShmBuffer()
{
    wl_buffer_destroy(buffer);
}

auto WaylandShmBuffer::use() -> wl_buffer*
{
    if (self_ptr)
    {
        mir::fatal_error("WaylandShmBuffer used multiple times");
    }
    self_ptr = shared_from_this();
    return buffer;
}

void WaylandShmBuffer::handle_release(void *data, wl_buffer*)
{
    auto const self = static_cast<WaylandShmBuffer*>(data);
    self->self_ptr.reset();
}

WaylandShm::WaylandShm(wl_shm* shm)
    : shm{shm}
{
}

auto WaylandShm::get_buffer(geom::Size size, geom::Stride stride) -> std::shared_ptr<WaylandShmBuffer>
{
    // If the old buffers have a different size, clear them all
    if (!buffers.empty() && (buffers[0]->size() != size || buffers[0]->stride() != stride))
    {
        buffers.clear();
    }

    std::shared_ptr<WaylandShmBuffer> free_buffer;

    // We can now assume all buffers in the list are the correct size, and look for a free one
    for (auto const& buffer : buffers)
    {
        if (!buffer->is_in_use())
        {
            free_buffer = buffer;
            break;
        }
    }

    // If we don't find one, we create a new buffer with a single-use pool
    if (!free_buffer)
    {
        void* data;
        size_t const data_size = size.height.as_int() * stride.as_int();
        auto const pool_resource = make_shm_pool(shm, data_size, &data);
        auto const pool = std::make_shared<WaylandShmPool>(pool_resource, data, data_size);
        auto const buffer = wl_shm_pool_create_buffer(
            pool_resource,
            0,
            size.width.as_int(),
            size.height.as_int(),
            stride.as_int(),
            WL_SHM_FORMAT_ARGB8888);
        free_buffer = std::make_shared<WaylandShmBuffer>(pool, data, size, stride, std::move(buffer));
        buffers.push_back(free_buffer);
    }

    return free_buffer;
}
