#ifndef WINDOW_GROUP_HPP
#define WINDOW_GROUP_HPP

#include "mir/geometry/forward.h"
#include "miral/window_info.h"
#include "miral/zone.h"
#include <cstddef>
#include <memory>
#include <vector>

/** Defines how new windows will be placed in the WindowGroup. */
enum class PlacementStrategy {
    /** If horizontal, we will place the new window to the right of the selectd window. */
    Horizontal,
    /** If vertical, we will place the new window below the selected window. */
    Vertical
};

/** Represents a group of windows in the tiler. */
class WindowGroup {
public:
    WindowGroup();
    WindowGroup(mir::geometry::Rectangle, PlacementStrategy strategy);
    ~WindowGroup();

    miral::Zone getZone();
    int getZoneId();
    void addWindow(std::shared_ptr<miral::Window>);
    void removeWindow(std::shared_ptr<miral::Window>);
    std::vector<std::shared_ptr<miral::Window>> getWindowsInZone();
    size_t getNumWindowsInGroup();

    /** Creates a group out of the */
    std::shared_ptr<WindowGroup> createSubGroup(const miral::Window& window, PlacementStrategy strategy);

    PlacementStrategy getPlacementStrategy();

private:
    miral::Zone mZone;
    std::vector<WindowGroup> mSubGroups;
    std::vector<std::shared_ptr<miral::Window>> mWindowsInZone;
    PlacementStrategy mPlacementStrategy;
};

#endif