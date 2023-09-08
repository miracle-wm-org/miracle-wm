//
// Created by mattkae on 9/8/23.
//

#ifndef MIRCOMPOSITOR_TILED_NODE_H
#define MIRCOMPOSITOR_TILED_NODE_H

#include <miral/window.h>
#include <mir/geometry/rectangle.h>

namespace mirie
{
class TilingRegion;

class TiledNode
{
public:
    TiledNode(miral::Window const&, std::shared_ptr<TilingRegion> const&);
    auto get_rectangle() -> mir::geometry::Rectangle;
    void update_region(std::shared_ptr<TilingRegion> const&);
private:
    miral::Window window;
    std::shared_ptr<TilingRegion> region;
};
}


#endif //MIRCOMPOSITOR_TILED_NODE_H
