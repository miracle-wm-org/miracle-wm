//
// Created by mattkae on 9/8/23.
//

#ifndef MIRCOMPOSITOR_TILING_REGION_H
#define MIRCOMPOSITOR_TILING_REGION_H

#include "tiled_node.h"

#include <mir/geometry/rectangle.h>
#include <miral/window.h>
#include <miral/application.h>
#include <memory>
#include <vector>

namespace mirie
{

enum class TilingRegionDirection
{
    horizontal,
    vertical,
    length
};

class TilingRegion
{
public:
    explicit TilingRegion(mir::geometry::Rectangle const&);
    void split(TilingRegionDirection direction);

private:
    mir::geometry::Rectangle rectangle;
    std::vector<TiledNode> windows;
    std::vector<std::shared_ptr<TilingRegion>> sub_regions;
};
}


#endif //MIRCOMPOSITOR_TILING_REGION_H
