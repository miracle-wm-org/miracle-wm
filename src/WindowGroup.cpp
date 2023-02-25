#include "WindowGroup.hpp"
#include "mir/geometry/forward.h"
#include "miral/window.h"
#include "miral/zone.h"
#include <algorithm>
#include <bits/ranges_algobase.h>
#include <bits/ranges_util.h>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

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
    std::cout << "Decounstructing " << getZoneId() << std::endl;
}

miral::Zone WindowGroup::getZone() {
    return mZone;
}

int WindowGroup::getZoneId() {
    return mZone.id();
}

std::shared_ptr<WindowGroup> WindowGroup::addWindow(std::shared_ptr<miral::Window> window) {
    // We don't have a root window
    if (!mWindow.get() && mWindowGroups.size() == 0) {
        std::cout << "Adding window as root window in the group" << std::endl;
        mWindow = window;
        return shared_from_this();
    }

    if (mWindowGroups.size() > 0 && mWindow.get()) {
        throw new std::exception();
    }
    
    auto controllingWindowGroup = getControllingWindowGroup();

    // If we are controlling ourselves AND we are just a single window, we need to go "amoeba-mode" and create
    // a new tile from the parent tile.
    if (controllingWindowGroup == shared_from_this() && mWindow) {
        std::cout << "Creating a new window group from the previous window." << std::endl;
        auto firstNewWindowGroup = controllingWindowGroup->makeWindowGroup(mWindow, PlacementStrategy::Parent);
        firstNewWindowGroup->mParent = shared_from_this();
        controllingWindowGroup->mWindowGroups.push_back(firstNewWindowGroup);
        mWindow.reset();
    }

    // Add the new window.
    std::cout << "Creating a new window group from the new window." << std::endl;
    auto secondNewWindowGroup = controllingWindowGroup->makeWindowGroup(window, PlacementStrategy::Parent);
    secondNewWindowGroup->mParent = shared_from_this();
    controllingWindowGroup->mWindowGroups.push_back(secondNewWindowGroup);
    return secondNewWindowGroup;
}

size_t WindowGroup::getNumTilesInGroup() {
    if (mWindow.get()) {
        return 1;
    }

    return mWindowGroups.size();
}

bool WindowGroup::removeWindow(std::shared_ptr<miral::Window> window) {
    if (mWindow == window) {
        // If this group represents the window, remove it.
        mWindow.reset();
        return true;
    }
    else {
        // Otherwise, search the other groups to remove it.
        for (auto group: mWindowGroups) {
            if (group->removeWindow(window)) {
                // If our child was the removed window, remove the child and steal his children.
                std::vector<std::shared_ptr<WindowGroup>>::iterator it = std::find(
                    mWindowGroups.begin(), mWindowGroups.end(), group);
                if(it != mWindowGroups.end()) {
                    for (auto adoptedWindowGroups : it->get()->mWindowGroups) {
                        mWindowGroups.push_back(adoptedWindowGroups);
                    }
                    std::cout << "Erasing window group from the parent. Size = " << mWindowGroups.size() << std::endl;
                    mWindowGroups.erase(it);
                }
                return true;
            }
        }
        return false;
    }
}

std::shared_ptr<WindowGroup> WindowGroup::makeWindowGroup(std::shared_ptr<miral::Window> window, PlacementStrategy placementStrategy) {
    // Capture the size of the window to make it the size of the new group.
    auto nextGroupPosition = window->top_left();
    auto nextGroupSize = window->size();
    auto zoneSize = mir::geometry::Rectangle(nextGroupPosition, nextGroupSize);
    auto windowGroup = std::make_shared<WindowGroup>(zoneSize, placementStrategy);
    windowGroup->mWindow = window;
    return windowGroup;
}

PlacementStrategy WindowGroup::getPlacementStrategy() {
    return mPlacementStrategy;
}

void WindowGroup::setPlacementStrategy(PlacementStrategy strategy) {
    mPlacementStrategy = strategy;
}

std::shared_ptr<WindowGroup> WindowGroup::getControllingWindowGroup() {
    if (mPlacementStrategy == PlacementStrategy::Parent) {
        return mParent->getControllingWindowGroup();
    }

    return shared_from_this();
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
        for (auto otherWindow : otherRetval) {
            retval.push_back(otherWindow);
        }
    }

    return retval;
}

std::shared_ptr<WindowGroup> WindowGroup::getParent() {
    if (mParent.get()) return mParent;
    else return shared_from_this();
}

std::shared_ptr<WindowGroup> WindowGroup::getWindowGroupForWindow(std::shared_ptr<miral::Window> window) {
    if (mWindow == window) {
        return shared_from_this();
    }
    else {
        // Otherwise, search the other groups to remove it.
        for (auto group: mWindowGroups) {
            auto windowGroup = group->getWindowGroupForWindow(window);
            if (windowGroup) {
                return windowGroup;
            }
        }
        return nullptr;
    }
}