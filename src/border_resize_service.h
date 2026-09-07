/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MIRACLE_BORDER_RESIZE_SERVICE_H
#define MIRACLE_BORDER_RESIZE_SERVICE_H

#include <memory>
#include <mir/geometry/rectangle.h>
#include <mir_toolkit/common.h>
#include <mir_toolkit/events/enums.h>
#include <string>

namespace miracle
{
namespace geom = mir::geometry;

class Config;
class CompositorState;
class OutputManager;
class WindowController;
class WindowContainer;
class CursorOverrideController;
class ResizeService;

/// The reach, in pixels, of the square at each corner of a window within which a
/// single edge is promoted to the neighbouring diagonal. Matches the value that
/// Mir's own server-side decorations use for their corner grab handles.
constexpr int border_resize_corner_size = 16;

/// Computes the resize edge for a point in a container's non-client band.
///
/// The band is everything inside \p logical - which includes half of the inner gap -
/// that falls outside of the client's content rectangle, i.e. \p visible shrunk by
/// \p border_size on every side. That band is exactly the region Mir does not deliver
/// to the client, because [mir::scene::BasicSurface::input_area_contains] tests the
/// content rectangle and so excludes the window margins that miracle draws its borders
/// in. Restricting ourselves to it means we never steal a click from a client.
///
/// \param allow_diagonals whether corners may be reported. Tiled containers pass
///                        false, because [Container::execute_resize] only acts on
///                        cardinal edges and would silently drop a diagonal.
/// \returns the edge under the point, or `mir_resize_edge_none`
MirResizeEdge border_edge_at(
    geom::Rectangle const& logical,
    geom::Rectangle const& visible,
    int border_size,
    bool allow_diagonals,
    float x,
    float y);

/// The cursor name from `mir_toolkit/cursors.h` that indicates \p edge.
std::string const& cursor_name_for_edge(MirResizeEdge edge);

/// Turns the window border into a resize handle.
///
/// Hovering an edge shows the matching resize cursor and pressing the primary button
/// there hands off to [ResizeService], which already knows how to drive both floating
/// and tiled resizes. This is the equivalent of what Mir's server-side decorations do
/// in `mir::shell::decoration::InputManager`, except that miracle owns no decoration
/// surface: the hit test runs against the container tree, and the cursor is forced
/// through [CursorOverrideController].
class BorderResizeService
{
public:
    BorderResizeService(
        std::shared_ptr<Config> const& config,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<OutputManager> const& output_manager,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CursorOverrideController> const& cursor_override,
        ResizeService& resize_service);

    /// \returns true if the event was consumed
    bool handle_pointer_event(float x, float y, MirPointerAction action, MirPointerButtons buttons);

    /// Drops any cursor override that is currently in effect.
    void clear();

private:
    /// \returns the container under the point along with the edge of it that the
    ///          point lies on, or a null container when the point is not on a
    ///          resizable border
    std::pair<std::shared_ptr<WindowContainer>, MirResizeEdge> resolve(float x, float y);

    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CursorOverrideController> cursor_override;
    ResizeService& resize_service;
};

} // namespace miracle

#endif // MIRACLE_BORDER_RESIZE_SERVICE_H
