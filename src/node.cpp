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

#define MIR_LOG_COMPONENT "node"

#include "node.h"
#include "leaf_node.h"
#include "node_common.h"
#include "parent_node.h"

using namespace miracle;

Node::Node(std::shared_ptr<ParentNode> const& parent) :
    parent { parent }
{
}

std::shared_ptr<LeafNode> Node::as_leaf(std::shared_ptr<Node> const& node)
{
    return std::dynamic_pointer_cast<LeafNode>(node);
}

std::shared_ptr<ParentNode> Node::as_lane(std::shared_ptr<Node> const& node)
{
    return std::dynamic_pointer_cast<ParentNode>(node);
}

bool Node::is_leaf()
{
    return as_leaf(shared_from_this()) != nullptr;
}

bool Node::is_lane()
{
    return as_lane(shared_from_this()) != nullptr;
}

std::weak_ptr<ParentNode> Node::get_parent() const
{
    return parent;
}

namespace
{
bool has_neighbor(Node const* node, NodeLayoutDirection direction, size_t cannot_be_index)
{
    auto shared_parent = node->get_parent().lock();
    if (!shared_parent)
        return false;

    if (shared_parent->get_direction() != direction)
        return has_neighbor(shared_parent.get(), direction, cannot_be_index);

    auto index = shared_parent->get_index_of_node(node);
    return (shared_parent->num_nodes() > 1 && index != cannot_be_index)
        || has_neighbor(shared_parent.get(), direction, cannot_be_index);
}

bool has_right_neighbor(Node const* node)
{
    auto shared_parent = node->get_parent().lock();
    if (!shared_parent)
        return false;
    return has_neighbor(node, NodeLayoutDirection::horizontal, shared_parent->num_nodes() - 1);
}

bool has_bottom_neighbor(Node const* node)
{
    auto shared_parent = node->get_parent().lock();
    if (!shared_parent)
        return false;
    return has_neighbor(node, NodeLayoutDirection::vertical, shared_parent->num_nodes() - 1);
}

bool has_left_neighbor(Node const* node)
{
    auto shared_parent = node->get_parent().lock();
    if (!shared_parent)
        return false;
    return has_neighbor(node, NodeLayoutDirection::horizontal, 0);
}

bool has_top_neighbor(Node const* node)
{
    auto shared_parent = node->get_parent().lock();
    if (!shared_parent)
        return false;
    return has_neighbor(node, NodeLayoutDirection::vertical, 0);
}
}

std::array<bool, (size_t)Direction::MAX> Node::get_neighbors() const
{
    return {
        has_top_neighbor(this),
        has_left_neighbor(this),
        has_bottom_neighbor(this),
        has_right_neighbor(this)
    };
}
