//
// Created by matthew on 7/25/23.
//

#include "tile_manager.h"
#include "tile.h"

miracle::TileManager::TileManager()
{
}

miracle::TileManager::~TileManager()
{
}

std::shared_ptr<miracle::Tile> miracle::TileManager::create_tile(mir::geometry::Rectangle rectangle)
{
    std::shared_ptr<Tile> tile = std::make_shared<Tile>(rectangle, this);
    m_tile_list.push_back(tile);
    return tile;
}