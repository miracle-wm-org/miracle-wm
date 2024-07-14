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

#ifndef MIRACLE_WM_STUB_SESSION_H
#define MIRACLE_WM_STUB_SESSION_H

#include <mir/scene/session.h>

namespace miracle::test
{
class StubSession : public mir::scene::Session
{
public:
    [[nodiscard]] auto process_id() const -> pid_t override
    {
        return 0;
    }

    [[nodiscard]] auto socket_fd() const -> mir::Fd override
    {
        return mir::Fd();
    }

    [[nodiscard]] auto name() const -> std::string override
    {
        return std::string();
    }

    void send_error(mir::ClientVisibleError const& error) override
    {
    }

    void send_input_config(MirInputConfig const& config) override
    {
    }

    [[nodiscard]] auto default_surface() const -> std::shared_ptr<mir::scene::Surface> override
    {
        return std::shared_ptr<mir::scene::Surface>();
    }

    void set_lifecycle_state(MirLifecycleState state) override
    {
    }

    void hide() override
    {
    }

    void show() override
    {
    }

    void start_prompt_session() override
    {
    }

    void stop_prompt_session() override
    {
    }

    void suspend_prompt_session() override
    {
    }

    void resume_prompt_session() override
    {
    }

    auto create_surface(std::shared_ptr<Session> const& session,
        mir::wayland::Weak<mir::frontend::WlSurface> const& wayland_surface,
        mir::shell::SurfaceSpecification const& params,
        std::shared_ptr<mir::scene::SurfaceObserver> const& observer,
        mir::Executor* observer_executor) -> std::shared_ptr<mir::scene::Surface> override
    {
        return std::shared_ptr<mir::scene::Surface>();
    }

    void destroy_surface(std::shared_ptr<mir::scene::Surface> const& surface) override
    {
    }

    [[nodiscard]] auto surface_after(std::shared_ptr<mir::scene::Surface> const& surface) const -> std::shared_ptr<mir::scene::Surface> override
    {
        return std::shared_ptr<mir::scene::Surface>();
    }

    auto create_buffer_stream(
        mir::graphics::BufferProperties const& props) -> std::shared_ptr<mir::compositor::BufferStream> override
    {
        return std::shared_ptr<mir::compositor::BufferStream>();
    }

    void destroy_buffer_stream(std::shared_ptr<mir::frontend::BufferStream> const& stream) override
    {
    }

    void configure_streams(mir::scene::Surface& surface, std::vector<mir::shell::StreamSpecification> const& config) override
    {
    }

public:
};
}

#endif // MIRACLE_WM_STUB_SESSION_H
