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
#include <mir/geometry/size.h>
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

enum class WindowResizeDirection
{
    up,
    left,
    down,
    right
};

/// Represents a tiling tree for an output.
class WindowTree
{
public:
    WindowTree(geom::Size default_size);
    ~WindowTree() = default;

    /// Makes space for the new window and returns its specified spot in the world.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification &requested_specification);

    /// Confirms the position of this window in the previously allocated position.
    void confirm(miral::Window&);

    void toggle_resize_mode();
    bool try_resize_active_window(WindowResizeDirection direction);
    void resize(geom::Size new_size);


    // Request a change to vertical window placement
    void request_vertical();

    // Request a change to horizontal window placement
    void request_horizontal();

    void advise_focus_gained(miral::Window&);
    void advise_focus_lost(miral::Window&);
    void advise_delete_window(miral::Window&);

private:
    void handle_direction_request(NodeDirection direction);
    std::shared_ptr<Node> root_lane;
    std::shared_ptr<Node> active_lane;
    miral::Window active_window;
    geom::Size size;
    bool is_resizing = false;

    void resize_node_to(std::shared_ptr<Node> node);
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H
