#ifndef WINDOW_GROUP_HPP
#define WINDOW_GROUP_HPP

#include "mir/geometry/forward.h"
#include "miral/window_info.h"
#include "miral/zone.h"
#include <cstddef>
#include <memory>
#include <vector>

/** Represents a group of windows in the tiler. */
class WindowGroup {
public:
    WindowGroup();
    WindowGroup(mir::geometry::Rectangle);
    ~WindowGroup();

    miral::Zone getZone();
    int getZoneId();
    void addWindow(std::shared_ptr<miral::Window>);
    void removeWindow(std::shared_ptr<miral::Window>);
    std::vector<std::shared_ptr<miral::Window>> getWindowsInZone();
    size_t getNumWindowsInGroup();
    std::shared_ptr<WindowGroup> createSubGroup(const miral::Window& window);

private:
    miral::Zone mZone;
    std::vector<WindowGroup> mSubGroups;
    std::vector<std::shared_ptr<miral::Window>> mWindowsInZone;
};

#endif