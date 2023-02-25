#ifndef WINDOW_GROUP_HPP
#define WINDOW_GROUP_HPP

#include "mir/geometry/forward.h"
#include "miral/window_info.h"
#include "miral/window_specification.h"
#include "miral/zone.h"
#include <cstddef>
#include <memory>
#include <vector>

/** Defines how new windows will be placed in the WindowGroup. */
enum class PlacementStrategy {
    /** If horizontal, we will place the new window to the right of the selectd window. */
    Horizontal,
    /** If vertical, we will place the new window below the selected window. */
    Vertical,
    /** If none was defined for this tile, we will get the nearest defined parent's. */
    Parent
};

/**
    Each WindowGroup represents a Tilelable object on the desktop.
    The smallest window group is comprised of a single window.
    A large WindowGroup is made up of many WindowGroups.
*/
class WindowGroup {
public:
    WindowGroup();
    WindowGroup(mir::geometry::Rectangle, PlacementStrategy strategy);
    ~WindowGroup();

    miral::Zone getZone();
    int getZoneId();

    /**
    Adds a window to the WindowGroup.
    
    @returns a pointer to the WindowGroup that the window now exists in.
    */
    WindowGroup* addWindow(std::shared_ptr<miral::Window>);

    /**
    Removes a window from the window group.

    @returns True if the window was removed, otherwise false.
    */
    bool removeWindow(std::shared_ptr<miral::Window>);
    std::vector<std::shared_ptr<miral::Window>> getWindowsInZone();
    size_t getNumTilesInGroup();

    PlacementStrategy getPlacementStrategy();

    /** Returns the window group who is in charge of organizing this window. */
    WindowGroup* getControllingWindowGroup();

    /** Returns true if the window group is the parent AND nothing has been added to it. */
    bool isEmpty();

private:
    miral::Zone mZone;

    std::shared_ptr<miral::Window> mWindow;
    std::shared_ptr<WindowGroup> mParent;
    std::vector<std::shared_ptr<WindowGroup>> mWindowGroups;
    PlacementStrategy mPlacementStrategy;

    std::shared_ptr<WindowGroup> makeWindowGroup(std::shared_ptr<miral::Window>, PlacementStrategy strategy);
};

#endif