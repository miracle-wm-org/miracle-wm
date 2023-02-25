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
    The smallest node is comprised of a single window.
    A large TileNode is made up of many TileNodes.
*/
class TileNode : public std::enable_shared_from_this<TileNode> {
public:
    TileNode(mir::geometry::Rectangle, PlacementStrategy strategy);
    ~TileNode();

    /**
        Retrieve the zone defined by this TileNode.
    */
    miral::Zone getZone();

    /**
        Retrieve the unique identifier for this zone.
    */
    int getZoneId();

    /**
        Retrieve the child nodes of this group.
    */
    std::vector<std::shared_ptr<TileNode>> getSubTileNodes();

    /**
        Adds a window to the TileNode.
        
        @returns a pointer to the TileNode that the window now exists in.
    */
    std::shared_ptr<TileNode> addWindow(std::shared_ptr<miral::Window>);

    /**
        Removes a window from the ToleNode.

        @returns True if the window was removed, otherwise false.
    */
    bool removeWindow(std::shared_ptr<miral::Window>);

    /**
        Collects all of the windows in the TileNode, including those in child nodes.
    */
    std::vector<std::shared_ptr<miral::Window>> getWindowsInTile();

    /**
        Gets the number of tiles under the control of this node EXCLUDING child nodes.
    */
    size_t getNumberOfTiles();

    /**
        Retrieves the placement strategy of this node.
    */
    PlacementStrategy getPlacementStrategy();

    /**
        Sets the placement strategy of this node.
    */
    void setPlacementStrategy(PlacementStrategy strategy);

    /**
        Returns the TileNode who is in charge of organizing this TileNode.
        This COULD be itself.
    */
    std::shared_ptr<TileNode> getControllingTileNode();

    /**
        Returns true if the TileNode is the parent AND nothing has been added to it.
    */
    bool isEmpty();

    /**
        Retrieves the parent of this child node. Expected to be nullptr for the root.
    */
    std::shared_ptr<TileNode> getParent();

    /**
        Given a window, searches recursively for the TileNode that holds it. Returns nullptr
        if none is found.
    */
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