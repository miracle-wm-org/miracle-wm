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

#ifndef MOCK_SURFACE_H
#define MOCK_SURFACE_H

#include "mir_version_manager.h"
#include <gmock/gmock.h>
#include <mir/scene/surface.h>

namespace miracle
{
namespace test
{
    class MockSurface : public mir::scene::Surface
    {
    public:
        MOCK_METHOD(void, initial_placement_done, (), (override));
        MOCK_METHOD(mir::geometry::Displacement, content_offset, (), (const, override));
        MOCK_METHOD(bool, input_area_contains, (mir::geometry::Point const&), (const, override));
        MOCK_METHOD(mir::input::InputReceptionMode, reception_mode, (), (const, override));
        MOCK_METHOD(void, consume, (std::shared_ptr<MirEvent const> const&), (override));
        MOCK_METHOD(bool, visible_on_lock_screen, (), (const, override));
        MOCK_METHOD(void, register_interest, (std::weak_ptr<mir::scene::SurfaceObserver> const&), (override));
        MOCK_METHOD(void, register_interest, (std::weak_ptr<mir::scene::SurfaceObserver> const&, mir::Executor&), (override));
        MOCK_METHOD(void, register_early_observer, (std::weak_ptr<mir::scene::SurfaceObserver> const&, mir::Executor&), (override));
        MOCK_METHOD(void, unregister_interest, (mir::scene::SurfaceObserver const&), (override));
        MOCK_METHOD(std::string, name, (), (const, override));
        MOCK_METHOD(mir::geometry::Size, content_size, (), (const, override));
        MOCK_METHOD(mir::geometry::Rectangle, input_bounds, (), (const, override));
        MOCK_METHOD(mir::geometry::Point, top_left, (), (const, override));
        MOCK_METHOD(mir::geometry::Size, window_size, (), (const, override));
        MOCK_METHOD(mir::graphics::RenderableList, generate_renderables, (mir::compositor::CompositorID), (const, override));
        MOCK_METHOD(MirWindowType, type, (), (const, override));
        MOCK_METHOD(MirWindowState, state, (), (const, override));
        MOCK_METHOD(mir::scene::SurfaceStateTracker, state_tracker, (), (const, override));
        MOCK_METHOD(void, hide, (), (override));
        MOCK_METHOD(void, show, (), (override));
        MOCK_METHOD(bool, visible, (), (const, override));
        MOCK_METHOD(void, move_to, (mir::geometry::Point const&), (override));
        MOCK_METHOD(void, set_input_region, (std::vector<mir::geometry::Rectangle> const&), (override));
        MOCK_METHOD(std::vector<mir::geometry::Rectangle>, get_input_region, (), (const, override));
        MOCK_METHOD(void, resize, (mir::geometry::Size const&), (override));
        MOCK_METHOD(void, set_transformation, (glm::mat4 const&), (override));
        MOCK_METHOD(void, set_alpha, (float), (override));
        MOCK_METHOD(void, set_orientation, (MirOrientation), (override));
        MOCK_METHOD(void, set_cursor_image, (std::shared_ptr<mir::graphics::CursorImage> const&), (override));
        MOCK_METHOD(std::shared_ptr<mir::graphics::CursorImage>, cursor_image, (), (const, override));
        MOCK_METHOD(void, set_reception_mode, (mir::input::InputReceptionMode), (override));
        MOCK_METHOD(void, request_client_surface_close, (), (override));
        MOCK_METHOD(std::shared_ptr<Surface>, parent, (), (const, override));
        MOCK_METHOD(int, configure, (MirWindowAttrib, int), (override));
        MOCK_METHOD(int, query, (MirWindowAttrib), (const, override));
        MOCK_METHOD(void, rename, (std::string const&), (override));
        MOCK_METHOD(void, set_streams, (std::list<mir::scene::StreamInfo> const&), (override));
        MOCK_METHOD(void, set_confine_pointer_state, (MirPointerConfinementState), (override));
        MOCK_METHOD(MirPointerConfinementState, confine_pointer_state, (), (const, override));
        MOCK_METHOD(void, placed_relative, (mir::geometry::Rectangle const&), (override));
        MOCK_METHOD(MirDepthLayer, depth_layer, (), (const, override));
        MOCK_METHOD(void, set_depth_layer, (MirDepthLayer), (override));
        MOCK_METHOD(void, set_visible_on_lock_screen, (bool), (override));
        MOCK_METHOD(std::optional<mir::geometry::Rectangle>, clip_area, (), (const, override));
        MOCK_METHOD(void, set_clip_area, (std::optional<mir::geometry::Rectangle> const&), (override));
        MOCK_METHOD(MirWindowFocusState, focus_state, (), (const, override));
        MOCK_METHOD(void, set_focus_state, (MirWindowFocusState), (override));
        MOCK_METHOD(std::string, application_id, (), (const, override));
        MOCK_METHOD(void, set_application_id, (std::string const&), (override));
        MOCK_METHOD(std::weak_ptr<mir::scene::Session>, session, (), (const, override));
        MOCK_METHOD(void, set_window_margins, (mir::geometry::DeltaY, mir::geometry::DeltaX, mir::geometry::DeltaY, mir::geometry::DeltaX), (override));
        MOCK_METHOD(MirFocusMode, focus_mode, (), (const, override));
        MOCK_METHOD(void, set_focus_mode, (MirFocusMode), (override));
        MOCK_METHOD(mir::Flags<MirTiledEdge>, tiled_edges, (), (const, override));
        MOCK_METHOD(void, set_tiled_edges, (mir::Flags<MirTiledEdge>), (override));
        MOCK_METHOD(std::list<mir::scene::StreamInfo>, get_streams, (), (const, override));
        MOCK_METHOD(void, set_mirror_mode, (MirMirrorMode), (override));
        MOCK_METHOD(mir::geometry::Width, min_width, (), (const, override));
        MOCK_METHOD(mir::geometry::Width, max_width, (), (const, override));
        MOCK_METHOD(mir::geometry::Height, min_height, (), (const, override));
        MOCK_METHOD(mir::geometry::Height, max_height, (), (const, override));
        MOCK_METHOD(void, set_min_width, (mir::geometry::Width), (override));
        MOCK_METHOD(void, set_max_width, (mir::geometry::Width), (override));
        MOCK_METHOD(void, set_min_height, (mir::geometry::Height), (override));
        MOCK_METHOD(void, set_max_height, (mir::geometry::Height), (override));
#ifdef MIR_VERSION_2_24_OR_GREATER
        MOCK_METHOD(void, set_opaque_region, (mir::geometry::Rectangles const&), (override));
#else
        MOCK_METHOD((mir::wayland::Weak<mir::frontend::WlSurface> const&), wayland_surface, (), (override));
#endif
    };

}
}

#endif // MOCK_SURFACE_H
