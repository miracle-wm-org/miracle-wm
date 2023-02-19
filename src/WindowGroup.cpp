#include "WindowGroup.hpp"
#include "mir/geometry/forward.h"
#include "miral/window.h"
#include "miral/zone.h"
#include <bits/ranges_util.h>
#include <cstddef>
#include <memory>

WindowGroup::WindowGroup():
    mZone(mir::geometry::Rectangle())
{

}

WindowGroup::WindowGroup(mir::geometry::Rectangle rectangle):
    mZone(rectangle)
{
}

WindowGroup::~WindowGroup() {

}

miral::Zone WindowGroup::getZone() {
    return mZone;
}

int WindowGroup::getZoneId() {
    return mZone.id();
}

size_t WindowGroup::getNumWindowsInGroup() {
    return mWindowsInZone.size();
}

void WindowGroup::addWindow(std::shared_ptr<miral::Window> window) {
    mWindowsInZone.push_back(window);
}

void WindowGroup::removeWindow(std::shared_ptr<miral::Window> window) {
    auto toErase = std::ranges::find(mWindowsInZone, window);
    if (toErase != mWindowsInZone.end()) {
        mWindowsInZone.erase(toErase);
    }
}

std::vector<std::shared_ptr<miral::Window>> WindowGroup::getWindowsInZone() {
    return mWindowsInZone;
}

std::shared_ptr<WindowGroup> WindowGroup::createSubGroup(const miral::Window& activeWindow) {
    auto nextGroupPosition = activeWindow.top_left();
    auto nextGroupSize = activeWindow.size();
    auto zoneSize = mir::geometry::Rectangle(nextGroupPosition, nextGroupSize);

    auto windowGroup = WindowGroup(zoneSize);
    windowGroup.addWindow(std::make_shared<miral::Window>(activeWindow));
    mSubGroups.push_back(windowGroup);
    return std::make_shared<WindowGroup>(windowGroup);
}