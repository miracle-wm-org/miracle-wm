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

#ifndef MIRACLE_WINDOW_CONTAINER_H
#define MIRACLE_WINDOW_CONTAINER_H

#include "container.h"

namespace miracle
{

/// An intermediate container class for containers that directly hold a window.
///
/// This class extends [Container] with methods that are meaningful only for
/// containers that wrap an actual [miral::Window] (e.g. [LeafContainer],
/// [PluginManagedContainer], [ShellComponentContainer]).  [ParentContainer]
/// does not inherit from this class because it does not directly hold a window
/// and has no use for these methods.
class WindowContainer : public Container
{
public:
    ~WindowContainer() override = default;
    virtual void handle_ready() = 0;
    virtual void handle_modify(miral::WindowSpecification const&) = 0;
    virtual void handle_request_move(MirInputEvent const* input_event) = 0;
    virtual void on_open() = 0;
    virtual void on_focus_lost() = 0;
    virtual void on_move_to(mir::geometry::Point const& top_left) = 0;
    virtual void on_resize(mir::geometry::Size const& size) = 0;
    virtual mir::geometry::Rectangle confirm_placement(
        MirWindowState, mir::geometry::Rectangle const&)
        = 0;

    virtual bool resize(Direction direction, int pixels) = 0;
    virtual bool toggle_fullscreen() = 0;
    virtual bool select_next(Direction direction) = 0;
    virtual bool move(Direction direction) = 0;
    virtual bool move_by(Direction direction, int pixels) = 0;
    using Container::move_by; // un-hide Container::move_by(float, float)
    virtual bool move_to(Container& other) = 0;
    using Container::move_to; // un-hide Container::move_to(int, int, bool)
    virtual bool is_fullscreen() const = 0;
    virtual bool drag_start() = 0;
    virtual bool drag_stop() = 0;
    virtual void drag(int x, int y) = 0;
};

} // namespace miracle

#endif // MIRACLE_WINDOW_CONTAINER_H
