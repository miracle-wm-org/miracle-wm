//
// Created by mattkae on 9/8/23.
//

#include "tiled_node.h"

using namespace mirie;

TiledNode::TiledNode(
    miral::Window const& window,
    std::shared_ptr<TilingRegion> const& region):
    window{window},
    region{region}
{
}

auto TiledNode::get_rectangle() -> mir::geometry::Rectangle
{
    return mir::geometry::Rectangle(
        window.top_left(),
        window.size());
}

void TiledNode::update_region(const std::shared_ptr<TilingRegion>& in_region)
{
    region = in_region;
}