miracle-wm
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

#ifndef MIRACLEWM_LEAF_NODE_H
#define MIRACLEWM_LEAF_NODE_H

#include "node.h"
#include "tiling_interface.h"
#include "node_common.h"
#include <miral/window_manager_tools.h>
#include <miral/window.h>
#include <optional>

namespace geom = mir::geometry;

namespace miracle
{

class MiracleConfig;
class TilingWindowTree;

class LeafNodeInterface
{
public:
};

class LeafNode : public Node
{
public:
    LeafNode(
        TilingInterface& node_interface,
        geom::Rectangle area,
        std::shared_ptr<MiracleConfig> const& config,
        TilingWindowTree* tree,
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
    void toggle_fullscreen();
    bool is_fullscreen() const;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    [[nodiscard]] TilingWindowTree* get_tree() const { return tree; }
    [[nodiscard]] miral::Window& get_window() { return window; }
    void commit_changes() override;

private:
    TilingInterface& node_interface;
    geom::Rectangle logical_area;
    std::optional<geom::Rectangle> next_logical_area;
    std::shared_ptr<MiracleConfig> config;
    TilingWindowTree* tree;
    miral::Window window;
    std::optional<MirWindowState> before_shown_state;
    std::optional<MirWindowState> next_state;
    NodeLayoutDirection tentative_direction = NodeLayoutDirection::none;
};

} // miracle

#endif //MIRACLEWM_LEAF_NODE_H
