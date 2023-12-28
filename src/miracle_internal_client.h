#ifndef MIRACLE_INTERNAL_CLIENT_H
#define MIRACLE_INTERNAL_CLIENT_H

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
