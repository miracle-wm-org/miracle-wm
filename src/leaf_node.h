#ifndef MIRACLEWM_LEAF_NODE_H
#define MIRACLEWM_LEAF_NODE_H

#include "node.h"
#include "node_interface.h"
#include <miral/window_manager_tools.h>
#include <miral/window.h>

namespace geom = mir::geometry;

namespace miracle
{

class MiracleConfig;
class Tree;

class LeafNodeInterface
{
public:
};

class LeafNode : public Node
{
public:
    LeafNode(
        std::shared_ptr<NodeInterface> const& node_interface,
        geom::Rectangle area,
        std::shared_ptr<MiracleConfig> const& config,
        Tree* tree,
        std::shared_ptr<ParentNode> const& parent);

    void associate_to_window(miral::Window const&);
    [[nodiscard]] geom::Rectangle get_logical_area() const override;
    [[nodiscard]] geom::Rectangle get_visible_area() const;
    void set_logical_area(geom::Rectangle const& target_rect) override;
    void set_parent(std::shared_ptr<ParentNode> const&) override;
    void scale_area(double x, double y) override;
    void translate(int x, int y) override;
    void show();
    void hide();
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    [[nodiscard]] Tree* get_tree() const { return tree; }
    [[nodiscard]] miral::Window& get_window() { return window; }
    void commit_changes() override;

private:
    std::shared_ptr<NodeInterface> node_interface;
    geom::Rectangle logical_area;
    std::shared_ptr<MiracleConfig> config;
    Tree* tree;
    miral::Window window;
    bool is_shown = true;
};

} // miracle

#endif //MIRACLEWM_LEAF_NODE_H
