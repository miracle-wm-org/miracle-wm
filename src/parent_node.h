#ifndef MIRACLEWM_PARENT_NODE_H
#define MIRACLEWM_PARENT_NODE_H

#include "node_common.h"
#include "node_interface.h"
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class Node;
class LeafNode;
class MiracleConfig;
class Tree;

class ParentNode
{
public:
    ParentNode(std::shared_ptr<NodeInterface> const&,
        geom::Rectangle,
        std::shared_ptr<MiracleConfig> const&,
        Tree* tree,
        Node* parent);
    geom::Rectangle get_logical_area() const;
    size_t num_nodes() const;
    void create_space_for_window(int index = -1);
    void confirm_window(miral::Window const&);
    void set_logical_area(geom::Rectangle const& target_rect);
    void scale_area(double x_scale, double y_scale);
    void translate_by(int x, int y);
    void set_direction(NodeLayoutDirection direction);
    void swap_nodes(std::shared_ptr<Node> const& first, std::shared_ptr<Node> const& second);
    void remove(std::shared_ptr<Node> const& node);
    void restore(std::shared_ptr<Node> const& node);
    void minimize(std::shared_ptr<Node> const& node);
    void commit_changes();

private:
    std::shared_ptr<NodeInterface> node_interface;
    geom::Rectangle logical_area;
    Tree* tree;
    Node* parent;
    std::shared_ptr<MiracleConfig> config;
    NodeLayoutDirection direction = NodeLayoutDirection::horizontal;
    std::vector<std::shared_ptr<Node>> sub_nodes;
    std::shared_ptr<Node> pending_node;

    int get_index_of_node(std::shared_ptr<Node> const& node) const;
    void constrain();
    void relayout();
};

} // miracle

#endif //MIRACLEWM_PARENT_NODE_H
