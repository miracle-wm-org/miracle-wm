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

#include "container.h"
#include "node_common.h"
#include "window_controller.h"
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class LeafContainer;
class MiracleConfig;
class TilingWindowTree;

/// A parent container defines the layout of containers beneath it.
class ParentContainer : public Container
{
public:
    ParentContainer(WindowController&,
        geom::Rectangle,
        std::shared_ptr<MiracleConfig> const&,
        TilingWindowTree* tree,
        std::shared_ptr<ParentContainer> const& parent);
    geom::Rectangle get_logical_area() const override;
    geom::Rectangle get_visible_area() const override;
    size_t num_nodes() const;
    std::shared_ptr<LeafContainer> create_space_for_window(int index = -1);
    std::shared_ptr<LeafContainer> confirm_window(miral::Window const&);
    void graft_existing(std::shared_ptr<Container> const& node, int index);
    std::shared_ptr<ParentContainer> convert_to_parent(std::shared_ptr<Container> const& container);
    void set_logical_area(geom::Rectangle const& target_rect) override;
    void set_direction(NodeLayoutDirection direction);
    void swap_nodes(std::shared_ptr<Container> const& first, std::shared_ptr<Container> const& second);
    void remove(std::shared_ptr<Container> const& node);
    void commit_changes() override;
    std::shared_ptr<Container> at(size_t i) const;
    std::shared_ptr<LeafContainer> get_nth_window(size_t i) const;
    std::shared_ptr<Container> find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const;
    NodeLayoutDirection get_direction() { return direction; }
    std::vector<std::shared_ptr<Container>> const& get_sub_nodes() const;
    [[nodiscard]] int get_index_of_node(Container const* node) const;
    [[nodiscard]] int get_index_of_node(std::shared_ptr<Container> const& node) const;
    [[nodiscard]] int get_index_of_node(Container const&) const;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    void set_parent(std::shared_ptr<Container> const&) override;

private:
    WindowController& node_interface;
    geom::Rectangle logical_area;
    TilingWindowTree* tree;
    std::shared_ptr<MiracleConfig> config;
    NodeLayoutDirection direction = NodeLayoutDirection::horizontal;
    std::vector<std::shared_ptr<Container>> sub_nodes;
    std::shared_ptr<LeafContainer> pending_node;

    geom::Rectangle create_space(int pending_index);
    void relayout();
};

} // miracle

#endif // MIRACLEWM_PARENT_NODE_H
