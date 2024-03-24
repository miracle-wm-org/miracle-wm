#ifndef MIRACLEWM_LEAF_NODE_H
#define MIRACLEWM_LEAF_NODE_H

#include "node_interface.h"
#include <miral/window_manager_tools.h>
#include <miral/window.h>

namespace geom = mir::geometry;

namespace miracle
{

class MiracleConfig;
class Tree;
class Node;

class LeafNodeInterface
{
public:
};

class LeafNode
{
public:
    LeafNode(
        std::shared_ptr<NodeInterface> const& node_interface,
        geom::Rectangle area,
        std::shared_ptr<MiracleConfig> const& config,
        Tree* tree,
        Node* parent);

    void associate_to_window(miral::Window const&);
    geom::Rectangle get_logical_area() const;
    geom::Rectangle get_visible_area() const;
    void set_logical_area(geom::Rectangle const& target_rect);
    void show();
    void hide();
    void constrain();
    Tree* get_tree() const { return tree; }
    Node* get_parent() const { return parent; }
    miral::Window const& get_window() const { return window; }
    void commit_changes();

private:
    std::shared_ptr<NodeInterface> node_interface;
    geom::Rectangle logical_area;
    std::shared_ptr<MiracleConfig> config;
    Tree* tree;
    Node* parent;
    miral::Window window;
    bool is_shown = true;
};

} // miracle

#endif //MIRACLEWM_LEAF_NODE_H
