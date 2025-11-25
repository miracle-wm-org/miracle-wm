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

#ifndef MOCK_SESSION_H
#define MOCK_SESSION_H

#include "mir_version_manager.h"
#include <gmock/gmock.h>
#include <mir/scene/session.h>
#include <mir/shell/surface_specification.h>

namespace miracle
{
namespace test
{
    class MockSession : public mir::scene::Session
    {
    public:
        MOCK_METHOD(pid_t, process_id, (), (const, override));
        MOCK_METHOD(mir::Fd, socket_fd, (), (const, override));
        MOCK_METHOD(std::string, name, (), (const, override));
        MOCK_METHOD(std::shared_ptr<mir::scene::Surface>, default_surface, (), (const, override));
        MOCK_METHOD(void, hide, (), (override));
        MOCK_METHOD(void, show, (), (override));
        MOCK_METHOD(void, start_prompt_session, (), (override));
        MOCK_METHOD(void, stop_prompt_session, (), (override));
        MOCK_METHOD(void, suspend_prompt_session, (), (override));
        MOCK_METHOD(void, resume_prompt_session, (), (override));
#ifdef MIR_VERSION_2_24_OR_GREATER
        MOCK_METHOD(std::shared_ptr<mir::scene::Surface>, create_surface,
            (std::shared_ptr<Session> const& session,
                mir::shell::SurfaceSpecification const& params,
                std::shared_ptr<mir::scene::SurfaceObserver> const& observer,
                mir::Executor* observer_executor),
            (override));
#else
        MOCK_METHOD(std::shared_ptr<mir::scene::Surface>, create_surface,
            (std::shared_ptr<Session> const& session,
                mir::wayland::Weak<mir::frontend::WlSurface> const& wayland_surface,
                mir::shell::SurfaceSpecification const& params,
                std::shared_ptr<mir::scene::SurfaceObserver> const& observer,
                mir::Executor* observer_executor),
            (override));
        MOCK_METHOD(void, send_error, (mir::ClientVisibleError const& error), (override));
#endif
        MOCK_METHOD(void, destroy_surface, (std::shared_ptr<mir::scene::Surface> const& surface), (override));
        MOCK_METHOD(std::shared_ptr<mir::scene::Surface>, surface_after,
            (std::shared_ptr<mir::scene::Surface> const& surface),
            (const, override));
        MOCK_METHOD(std::shared_ptr<mir::compositor::BufferStream>, create_buffer_stream,
            (mir::graphics::BufferProperties const& props),
            (override));
        MOCK_METHOD(void, destroy_buffer_stream, (std::shared_ptr<mir::frontend::BufferStream> const& stream), (override));
        MOCK_METHOD(void, configure_streams,
            (mir::scene::Surface & surface, std::vector<mir::shell::StreamSpecification> const& config),
            (override));
    };

}
}

#endif // MOCK_SESSION_H
