/**
Copyright (C) 2024  Matthew Kosarek

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

#ifndef MIRACLE_WM_FLOATING_CONTAINER_H
#define MIRACLE_WM_FLOATING_CONTAINER_H

#include "container.h"
#include <miral/minimal_window_manager.h>
#include <miral/window.h>

namespace miracle
{
class WindowController;

/// Contains a single float window
/// TODO: Allow access to an entire TilingWindowTree here
class FloatingContainer : public Container
{
public:
    explicit FloatingContainer(
        miral::Window const&,
        miral::MinimalWindowManager& wm,
        WindowController& window_controller);
    [[nodiscard]] mir::geometry::Rectangle get_logical_area() const override;
    void set_logical_area(mir::geometry::Rectangle const&) override;
    void commit_changes() override;
    mir::geometry::Rectangle get_visible_area() const override;
    void constrain() override;
    void set_parent(std::shared_ptr<Container> const& ptr) override;
    size_t get_min_height() const override;
    size_t get_min_width() const override;
    void handle_ready() const override;
    void handle_modify(miral::WindowSpecification const&) override;

    bool pinned() const;
    void pinned(bool);
    [[nodiscard]] miral::Window const& window() const;

private:
    miral::Window window_;
    miral::MinimalWindowManager& wm;
    WindowController& window_controller;
    bool is_pinned = false;
};

} // miracle

#endif //MIRACLE_WM_FLOATING_CONTAINER_H
