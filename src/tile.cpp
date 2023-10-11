//
// Created by matthew on 7/25/23.
//
#include "tile.h"
#include <miral/window.h>

miracle::Tile::Tile(mir::geometry::Rectangle rectangle, TileManager* tile_manager)
    : m_rectangle(rectangle), m_manager(tile_manager)
{
}

miracle::Tile::~Tile()
{
}

bool miracle::Tile::try_insert(std::shared_ptr<miral::Window> window)
{
    return false;
}

bool miracle::Tile::try_remove(std::shared_ptr<miral::Window> window)
{
    return false;
}