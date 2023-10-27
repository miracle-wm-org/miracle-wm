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

#ifndef MIRAL_WAYLAND_SHM_H
#define MIRAL_WAYLAND_SHM_H

#include "mir/geometry/size.h"

#include <wayland-client.h>
#include <memory>
#include <vector>

namespace miracle
{
class WaylandShm;
struct WaylandShmPool;

class WaylandShmBuffer : public std::enable_shared_from_this<WaylandShmBuffer>
{
public:
    WaylandShmBuffer(
        std::shared_ptr<WaylandShmPool> pool,
        void* data,
        mir::geometry::Size size,
        mir::geometry::Stride stride,
        wl_buffer* buffer);
    ~WaylandShmBuffer();

    auto data() const -> void* { return data_; }
    auto size() const -> mir::geometry::Size { return size_; }
    auto stride() const -> mir::geometry::Stride { return stride_; }
    /// Returns if this buffer is currently being used by the compositor. In-use buffers keep themselves alive.
    auto is_in_use() const -> bool { return self_ptr != nullptr; }
    /// Marks this buffer as in-use and assumes the resulting wl_buffer is sent to the compositor. Keeps this buffer
    /// alive until the compositor releases it (if it's not sent to the compositor or the compositor never releases it
    /// this buffer is leaked). Should only be called if the buffer is not already in-use.
    auto use() -> wl_buffer*;

private:
    friend WaylandShm;

    static wl_buffer_listener const buffer_listener;

    static void handle_release(void *data, wl_buffer*);

    std::shared_ptr<WaylandShmPool> const pool;
    void* const data_;
    mir::geometry::Size const size_;
    mir::geometry::Stride const stride_;
    wl_buffer* const buffer;
    std::shared_ptr<WaylandShmBuffer> self_ptr; ///< Is set on use and cleared on release
};

/// A single WaylandShm does not efficiently provision multiple buffers for multiple window sizes. Please use one
/// WaylandShm per window (if windows may have distinct sizes)
class WaylandShm
{
public:
    /// Does not take ownership of the wl_shm
    WaylandShm(wl_shm* shm);

    /// Always returns a buffer of the correct size that is not in-use
    auto get_buffer(mir::geometry::Size size, mir::geometry::Stride stride) -> std::shared_ptr<WaylandShmBuffer>;

private:
    wl_shm* const shm;
    /// get_buffer() assumes all buffers in the list have the same size and stride
    std::vector<std::shared_ptr<WaylandShmBuffer>> buffers;
};
}

#endif // MIRAL_WAYLAND_SHM_H
