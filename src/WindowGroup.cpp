#include "WindowGroup.hpp"
#include "mir/geometry/forward.h"
#include "miral/window.h"
#include "miral/zone.h"
#include <algorithm>
#include <bits/ranges_algobase.h>
#include <bits/ranges_util.h>
#include <cstddef>
#include <exception>
#include <memory>
#include <vector>

WindowGroup::WindowGroup():
    mZone(mir::geometry::Rectangle())
{
    mPlacementStrategy = PlacementStrategy::Horizontal;
}

WindowGroup::WindowGroup(mir::geometry::Rectangle rectangle, PlacementStrategy strategy):
    mZone(rectangle)
{
    mPlacementStrategy = strategy;
}

WindowGroup::~WindowGroup() {

}

miral::Zone WindowGroup::getZone() {
    return mZone;
}

int WindowGroup::getZoneId() {
    return mZone.id();
}

std::shared_ptr<WindowGroup> WindowGroup::addWindow(std::shared_ptr<miral::Window> window) {
    // We have a root window group.
    if (!mWindow.get() && mWindowGroups.size() == 0) {
        mWindow = window;
        return std::shared_ptr<WindowGroup>(this);
    }

    if (mWindowGroups.size() > 0 && mWindow.get()) {
        throw new std::exception();
    }
    
    // If this WindowGroup is just a window, make it a WindowGroup 
    if (mWindow.get()) {
        auto firstNewWindowGroup = makeWindowGroup(mWindow, PlacementStrategy::Parent);
        firstNewWindowGroup->mParent = std::shared_ptr<WindowGroup>(this);
        mWindowGroups.push_back(firstNewWindowGroup);
        mWindow.reset();
    }

    // Add the new window.
    auto secondNewWindowGroup = makeWindowGroup(window, PlacementStrategy::Parent);
    secondNewWindowGroup->mParent = std::shared_ptr<WindowGroup>(this);
    mWindowGroups.push_back(secondNewWindowGroup);
    return std::shared_ptr<WindowGroup>(secondNewWindowGroup);
}

size_t WindowGroup::getNumTilesInGroup() {
    if (mWindow.get()) {
        return 1;
    }

    return mWindowGroups.size();
}

void WindowGroup::removeWindow(std::shared_ptr<miral::Window> window) {
    // auto toErase = std::ranges::find(mWindowsInZone, window);
    // if (toErase != mWindowsInZone.end()) {
    //     mWindowsInZone.erase(toErase);
    // }
}

std::shared_ptr<WindowGroup> WindowGroup::makeWindowGroup(std::shared_ptr<miral::Window> window, PlacementStrategy placementStrategy) {
    // Capture the size of the window to make it the size of the new group.
    auto nextGroupPosition = window->top_left();
    auto nextGroupSize = window->size();
    auto zoneSize = mir::geometry::Rectangle(nextGroupPosition, nextGroupSize);
    return std::make_shared<WindowGroup>(zoneSize, placementStrategy);
}

PlacementStrategy WindowGroup::getPlacementStrategy() {
    return mPlacementStrategy;
}

WindowGroup* WindowGroup::getControllingWindowGroup() {
    if (mPlacementStrategy == PlacementStrategy::Parent) {
        return mParent->getControllingWindowGroup();
    }

    return this;
}

bool WindowGroup::isEmpty() {
    return mParent.get() == nullptr && mWindow.get() == nullptr && mWindowGroups.size() == 0;
}

std::vector<std::shared_ptr<miral::Window>> WindowGroup::getWindowsInZone() {
    std::vector<std::shared_ptr<miral::Window>> retval;
    if (mWindow.get()) {
        retval.push_back(mWindow);
    }

    for (auto windowGroup : mWindowGroups) {
        auto otherRetval = windowGroup->getWindowsInZone();
        std::ranges::move(retval, std::back_inserter(otherRetval));
    }

    return retval;
}