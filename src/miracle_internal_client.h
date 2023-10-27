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

#ifndef MIRCOMPOSITOR_MIRACLE_INTERNAL_CLIENT_H
#define MIRCOMPOSITOR_MIRACLE_INTERNAL_CLIENT_H

#include <memory>
#include <mutex>
#include <miral/window_manager_tools.h>
#include <wayland-client.h>

namespace miracle
{

/// Helper interface that will store the session internally
class MiracleInternalClient
{
public:
    virtual void operator()(struct wl_display* display) = 0;
    virtual void stop() = 0;
    virtual void operator()(std::weak_ptr<mir::scene::Session> const& session) = 0;

    auto session() const -> std::shared_ptr<mir::scene::Session>
    {
        std::lock_guard lock{mutex};
        return weak_session.lock();
    }

protected:
    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> weak_session;
};

}

#endif //MIRCOMPOSITOR_MIRACLE_INTERNAL_CLIENT_H
