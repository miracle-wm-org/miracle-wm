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

#ifndef MIRCOMPOSITOR_NODE_H
#define MIRCOMPOSITOR_NODE_H

#include <mir/geometry/rectangle.h>
#include <vector>
#include <memory>
#include <miral/window.h>

namespace geom = mir::geometry;

namespace miracle
{

enum class NodeState
{
    window ,
    lane
};

enum class NodeDirection
{
    horizontal,
    vertical
};

/// A node in the tree is either a single window or a lane.
class Node : public std::enable_shared_from_this<Node>
{
public:
    Node(geom::Rectangle);
    Node(geom::Rectangle, std::shared_ptr<Node> parent, miral::Window& window);

    /// The rectangle defined by the node can be retrieved dynamically
    /// by calculating the dimensions of the content in this node
    geom::Rectangle get_rectangle();

    void set_rectangle(geom::Rectangle target_rect);

    /// Walk the tree to find the lane that contains this window.
    std::shared_ptr<Node> find_node_for_window(miral::Window& window);

    /// Transform the window  in the list to a Node. Returns the
    /// new Node if the Window was found, otherwise null.
    std::shared_ptr<Node> window_to_node(miral::Window& window);

    std::shared_ptr<Node> parent;

    bool is_root() { return parent == nullptr; }
    bool is_window() { return state == NodeState::window; }
    bool is_lane() { return state == NodeState::lane; }
    NodeDirection get_direction() { return direction; }
    miral::Window const& get_window() { return window; }
    std::vector<std::shared_ptr<Node>>& get_sub_nodes() { return sub_nodes; }
    void set_direction(NodeDirection in_direction) { direction = in_direction; }

    void to_lane();

private:
    miral::Window window;
    std::vector<std::shared_ptr<Node>> sub_nodes;
    NodeState state;
    NodeDirection direction = NodeDirection::horizontal;
    geom::Rectangle area;
};
}


#endif //MIRCOMPOSITOR_NODE_H
