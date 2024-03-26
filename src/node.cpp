#define MIR_LOG_COMPONENT "node"

#include "node.h"
#include "node_common.h"
#include "parent_node.h"
#include "leaf_node.h"

using namespace miracle;

Node::Node(std::shared_ptr<ParentNode> const& parent) : parent{parent} {}

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

bool Node::_has_right_neighbor() const
{
    auto shared_parent = parent.lock();
    if (!shared_parent)
        return false;

    if (shared_parent->get_direction() != NodeLayoutDirection::horizontal)
        return shared_parent->_has_right_neighbor();

    auto index = shared_parent->get_index_of_node(this);
    return (shared_parent->num_nodes() > 1 && index != shared_parent->num_nodes() - 1)
        || shared_parent->_has_right_neighbor();
}

bool Node::_has_bottom_neighbor() const
{
    auto shared_parent = parent.lock();
    if (!shared_parent)
        return false;

    if (shared_parent->get_direction() != NodeLayoutDirection::vertical)
        return shared_parent->_has_bottom_neighbor();

    auto index = shared_parent->get_index_of_node(this);
    return (shared_parent->num_nodes() > 1 && index != shared_parent->num_nodes() - 1)
           || shared_parent->_has_bottom_neighbor();
}
