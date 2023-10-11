//
// Created by matthew on 7/25/23.
//

#ifndef MIRCOMPOSITOR_TILE_MANAGER_H
#define MIRCOMPOSITOR_TILE_MANAGER_H

#include <memory>
#include <vector>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/point.h>

namespace miracle
{
class Tile;

class TileManager
{
public:
    TileManager();
    ~TileManager();
    std::shared_ptr<Tile> create_tile(mir::geometry::Rectangle);
    bool try_remove_tile(std::shared_ptr<Tile>);
    std::shared_ptr<Tile> try_intersect_tile(mir::geometry::Point);

private:
    std::vector<std::shared_ptr<Tile>> m_tile_list;
};
}

#endif //MIRCOMPOSITOR_TILE_MANAGER_H
