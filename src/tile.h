//
// Created by matthew on 7/25/23.
//

#ifndef MIRCOMPOSITOR_TILE_H
#define MIRCOMPOSITOR_TILE_H

#include <mir/geometry/rectangle.h>
#include <vector>
#include <memory>

namespace miral
{
class Window;
}

namespace miracle
{
class TileManager;

enum class Direction
{
    Up,
    Down,
    Left,
    Right,
    Size
};

class Tile
{
public:
    /// \param rectangle Area of the screen defined by the tile
    /// \param tile_manager Used to request new tiles
    Tile(mir::geometry::Rectangle rectangle, TileManager* tile_manager);
    ~Tile();

    /// Insert a window into the tile
    bool try_insert(std::shared_ptr<miral::Window>);

    /// Remove a window from the tile
    bool try_remove(std::shared_ptr<miral::Window>);

    /// Make the provided window be a new tile
    std::shared_ptr<Tile> to_tile(std::shared_ptr<miral::Window>);

    /// Expand the window in the provided direction by the provided number of pixels
    bool expand_in(Direction, unsigned int);

    /// Swap the window with a window in a neighboring direction
    bool swap_with(Direction);

private:
    mir::geometry::Rectangle m_rectangle;
    TileManager* m_manager;
    std::vector<std::shared_ptr<miral::Window>> m_window_list;
    std::shared_ptr<Tile> m_parent_tile;
    std::vector<std::shared_ptr<Tile>> m_child_tile_list;
};
}

#endif //MIRCOMPOSITOR_TILE_H
