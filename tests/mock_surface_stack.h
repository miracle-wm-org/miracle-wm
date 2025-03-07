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

#ifndef MIRACLE_MOCK_SURFACE_STACK_H_
#define MIRACLE_MOCK_SURFACE_STACK_H_

#include <gmock/gmock.h>
#include <mir/shell/surface_stack.h>
#include <mir/version.h>

namespace miracle
{
namespace test
{
    struct MockSurfaceStack : public mir::shell::SurfaceStack
    {
        MOCK_METHOD1(raise, void(std::weak_ptr<mir::scene::Surface> const&));
        MOCK_METHOD1(raise, void(mir::scene::SurfaceSet const&));

        MOCK_METHOD2(add_surface, void(std::shared_ptr<mir::scene::Surface> const&, mir::input::InputReceptionMode new_mode));

        MOCK_METHOD1(remove_surface, void(std::weak_ptr<mir::scene::Surface> const& surface));
        MOCK_CONST_METHOD1(surface_at, std::shared_ptr<mir::scene::Surface>(mir::geometry::Point));
        MOCK_METHOD2(swap_z_order, void(mir::scene::SurfaceSet const&, mir::scene::SurfaceSet const&));
        MOCK_METHOD1(send_to_back, void(mir::scene::SurfaceSet const&));
#if MIR_SERVER_MAJOR_VERSION >= 2 && MIR_SERVER_MINOR_VERSION >= 20
        MOCK_CONST_METHOD2(is_above, bool(std::weak_ptr<mir::scene::Surface> const& a, std::weak_ptr<mir::scene::Surface> const& b));
#endif
    };
}
}

#endif
