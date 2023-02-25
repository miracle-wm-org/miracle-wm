#ifndef WINDOW_GROUP_HPP
#define WINDOW_GROUP_HPP

#include "mir/geometry/forward.h"
#include "miral/window.h"
#include "miral/window_info.h"
#include "miral/window_specification.h"
#include "miral/zone.h"
#include <cstddef>
#include <memory>
#include <vector>

/** Defines how new windows will be placed in the TileNode. */
enum class PlacementStrategy {
    /** If horizontal, we will place the new window to the right of the selectd window. */
    Horizontal,
    /** If vertical, we will place the new window below the selected window. */
    Vertical,
    /** If none was defined for this tile, we will get the nearest defined parent's. */
    Parent
};

/**
    Each TileNode represents a Tilelable object on the desktop.
    The smallest window group is comprised of a single window.
    A large TileNode is made up of many TileNodes.
*/
class TileNode : public std::enable_shared_from_this<TileNode> {
public:
    TileNode();
    TileNode(mir::geometry::Rectangle, PlacementStrategy strategy);
    ~TileNode();

    miral::Zone getZone();
    int getZoneId();

    /**
    Retrieve a vector of window groups within this window group.
    */
    std::vector<std::shared_ptr<TileNode>> getSubTileNodes();

    /**
    Adds a window to the TileNode.
    
    @returns a pointer to the TileNode that the window now exists in.
    */
    std::shared_ptr<TileNode> addWindow(std::shared_ptr<miral::Window>);

    /**
    Removes a window from the window group.

    @returns True if the window was removed, otherwise false.
    */
    bool removeWindow(std::shared_ptr<miral::Window>);
    std::vector<std::shared_ptr<miral::Window>> getWindowsInZone();
    size_t getNumTilesInGroup();

    PlacementStrategy getPlacementStrategy();
    void setPlacementStrategy(PlacementStrategy strategy);

    /** Returns the window group who is in charge of organizing this window. */
    std::shared_ptr<TileNode> getControllingTileNode();

    /** Returns true if the window group is the parent AND nothing has been added to it. */
    bool isEmpty();

    std::shared_ptr<TileNode> getParent();

    std::shared_ptr<TileNode> getTileNodeForWindow(std::shared_ptr<miral::Window>);

private:
    miral::Zone mZone;

    std::shared_ptr<miral::Window> mWindow;
    std::shared_ptr<TileNode> mParent;
    std::vector<std::shared_ptr<TileNode>> mTileNodes;
    PlacementStrategy mPlacementStrategy;

    std::shared_ptr<TileNode> makeTileNode(std::shared_ptr<miral::Window>, PlacementStrategy strategy);
};

#endif