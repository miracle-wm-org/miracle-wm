#ifndef MIRACLEWM_PARENT_NODE_H
#define MIRACLEWM_PARENT_NODE_H

#include "node_common.h"
#include "node_interface.h"
#include "node.h"
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class LeafNode;
class MiracleConfig;
class Tree;

class ParentNode : public Node
{
public:
    ParentNode(std::shared_ptr<NodeInterface> const&,
        geom::Rectangle,
        std::shared_ptr<MiracleConfig> const&,
        Tree* tree,
        std::shared_ptr<ParentNode> const& parent);
    geom::Rectangle get_logical_area() const override;
    size_t num_nodes() const;
    std::shared_ptr<LeafNode> create_space_for_window(int index = -1);
    std::shared_ptr<LeafNode> confirm_window(miral::Window const&);
    void graft_existing(std::shared_ptr<Node> const& node, int index);
    void convert_to_lane(std::shared_ptr<LeafNode> const&);
    void set_logical_area(geom::Rectangle const& target_rect) override;
    void scale_area(double x_scale, double y_scale) override;
    void translate(int x, int y) override;
    void set_direction(NodeLayoutDirection direction);
    void swap_nodes(std::shared_ptr<Node> const& first, std::shared_ptr<Node> const& second);
    void remove(std::shared_ptr<Node> const& node);
    void commit_changes() override;
    std::shared_ptr<Node> at(size_t i) const;
    std::shared_ptr<LeafNode> get_nth_window(size_t i) const;
    std::shared_ptr<Node> find_where(std::function<bool(std::shared_ptr<Node> const&)> func) const;
    NodeLayoutDirection get_direction() { return direction; }
    std::vector<std::shared_ptr<Node>> const& get_sub_nodes() const;
    int get_index_of_node(Node const* node) const;
    [[nodiscard]] int get_index_of_node(std::shared_ptr<Node> const& node) const;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    void set_parent(std::shared_ptr<ParentNode> const&) override;

private:
    std::shared_ptr<NodeInterface> node_interface;
    geom::Rectangle logical_area;
    Tree* tree;
    std::shared_ptr<MiracleConfig> config;
    NodeLayoutDirection direction = NodeLayoutDirection::horizontal;
    std::vector<std::shared_ptr<Node>> sub_nodes;
    std::shared_ptr<LeafNode> pending_node;

    geom::Rectangle create_space(int pending_index);
    void relayout();
};

} // miracle

#endif //MIRACLEWM_PARENT_NODE_H
