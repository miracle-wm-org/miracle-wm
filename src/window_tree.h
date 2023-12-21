/*
 * Copyright © Canonical Ltd.
 *
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

#ifndef MIRCOMPOSITOR_WINDOW_TREE_H
#define MIRCOMPOSITOR_WINDOW_TREE_H

#include "node.h"
#include <memory>
#include <vector>
#include <miral/window.h>
#include <miral/window_specification.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangle.h>
#include <miral/window_manager_tools.h>

namespace geom = mir::geometry;

namespace miracle
{

enum class Direction
{
    up,
    left,
    down,
    right
};

struct WindowTreeOptions
{
    int gap_x;
    int gap_y;
};

/// Represents a tiling tree for an output.
class WindowTree
{
public:
    WindowTree(geom::Rectangle area, miral::WindowManagerTools const& tools, WindowTreeOptions const& options);
    ~WindowTree() = default;

    /// Makes space for the new window and returns its specified spot in the world.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification &requested_specification);

    /// Confirms the position of this window in the previously allocated position.
    void confirm_new_window(miral::Window&);

    /// Places us into resize mode. Other operations are prohibited while we are in resize mode.
    void toggle_resize_mode();

    /// Try to resize the current active window in the provided direction
    bool try_resize_active_window(Direction direction);

    /// Move the active window in the provided direction
    bool try_move_active_window(Direction direction);

    /// Select the next window in the provided direction
    bool try_select_next(Direction direction);

    // Request a change to vertical window placement
    void request_vertical();

    // Request a change to horizontal window placement
    void request_horizontal();

    /// Advises us to focus the provided window.
    void advise_focus_gained(miral::Window&);

    /// Advises us to lose focus on the provided window.
    void advise_focus_lost(miral::Window&);

    /// Called when the window was deleted.
    void advise_delete_window(miral::Window&);

    /// Called when the physical display is resized.
    void set_output_area(geom::Rectangle new_area);

    bool point_is_in_output(int x, int y);

private:
    miral::WindowManagerTools tools;
    WindowTreeOptions options;
    std::shared_ptr<Node> root_lane;
    std::shared_ptr<Node> active_window;
    geom::Rectangle area;
    bool is_resizing = false;

    std::shared_ptr<Node> get_active_lane();
    void handle_direction_request(NodeLayoutDirection direction);
    void resize_node_in_direction(std::shared_ptr<Node> node, Direction direction, int amount);
    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    std::shared_ptr<Node> traverse(std::shared_ptr<Node> from, Direction direction);
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H
