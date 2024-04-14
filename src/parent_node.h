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

#ifndef MIRACLEWM_PARENT_NODE_H
#define MIRACLEWM_PARENT_NODE_H

#include "node_common.h"
#include "tiling_interface.h"
#include "node.h"
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class LeafNode;
class MiracleConfig;
class TilingWindowTree;

class ParentNode : public Node
{
public:
    ParentNode(TilingInterface&,
               geom::Rectangle,
               std::shared_ptr<MiracleConfig> const&,
               TilingWindowTree* tree,
               std::shared_ptr<ParentNode> const& parent);
    geom::Rectangle get_logical_area() const override;
    size_t num_nodes() const;
    std::shared_ptr<LeafNode> create_space_for_window(int index = -1);
    std::shared_ptr<LeafNode> confirm_window(miral::Window const&);
    void graft_existing(std::shared_ptr<Node> const& node, int index);
    void convert_to_lane(std::shared_ptr<LeafNode> const&);
    void set_logical_area(geom::Rectangle const& target_rect) override;
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
    TilingInterface& node_interface;
    geom::Rectangle logical_area;
    TilingWindowTree* tree;
    std::shared_ptr<MiracleConfig> config;
    NodeLayoutDirection direction = NodeLayoutDirection::horizontal;
    std::vector<std::shared_ptr<Node>> sub_nodes;
    std::shared_ptr<LeafNode> pending_node;

    geom::Rectangle create_space(int pending_index);
    void relayout();
};

} // miracle

#endif //MIRACLEWM_PARENT_NODE_H
