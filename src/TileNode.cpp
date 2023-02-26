#include "TileNode.hpp"
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

TileNode::TileNode(mir::geometry::Rectangle rectangle, PlacementStrategy strategy):
    std::enable_shared_from_this<TileNode>(),
    mZone(rectangle)
{
    mPlacementStrategy = strategy;
}

TileNode::~TileNode() {
    std::cout << "Decounstructing " << getZoneId() << std::endl;
}

miral::Zone TileNode::getZone() {
    return mZone;
}

int TileNode::getZoneId() {
    return mZone.id();
}

std::vector<std::shared_ptr<TileNode>> TileNode::getChildNodeList() {
    return mChildNodes;
}

std::shared_ptr<TileNode> TileNode::addWindow(std::shared_ptr<miral::Window> window) {
    // We don't have a root window
    if (!mWindow.get() && mChildNodes.size() == 0) {
        std::cout << "Adding window as root window in the group" << std::endl;
        mWindow = window;
        return shared_from_this();
    }

    if (mChildNodes.size() > 0 && mWindow.get()) {
        throw new std::exception();
    }
    
    auto controllingTileNode = getControllingTileNode();

    // If we are controlling ourselves AND we are just a single window, we need to go "amoeba-mode" and create
    // a new tile from the parent tile.
    if (controllingTileNode == shared_from_this() && mWindow) {
        std::cout << "Creating a new window group from the previous window." << std::endl;
        auto firstNewTileNode = controllingTileNode->makeTileNode(mWindow, PlacementStrategy::Parent);
        firstNewTileNode->mParent = shared_from_this();
        controllingTileNode->mChildNodes.push_back(firstNewTileNode);
        mWindow.reset();
    }

    // Add the new window.
    std::cout << "Creating a new window group from the new window." << std::endl;
    auto secondNewTileNode = controllingTileNode->makeTileNode(window, PlacementStrategy::Parent);
    secondNewTileNode->mParent = controllingTileNode;
    controllingTileNode->mChildNodes.push_back(secondNewTileNode);
    return secondNewTileNode;
}

size_t TileNode::getNumberOfTiles() {
    if (mWindow.get()) {
        return 1;
    }

    return mChildNodes.size();
}

bool TileNode::removeWindow(std::shared_ptr<miral::Window> window) {
    if (mWindow == window) {
        // If this group represents the window, remove it.
        mWindow.reset();

        // Remove the child from the parent and take its children.
        if (mParent.get()) {
            std::vector<std::shared_ptr<TileNode>>::iterator it = std::find(
                mParent->mChildNodes.begin(), mParent->mChildNodes.end(), shared_from_this());
            if (it != mParent->mChildNodes.end()) {
                for (auto adoptedTileNodes : mChildNodes) {
                    mParent->mChildNodes.push_back(adoptedTileNodes);
                }
                std::cout << "Erasing window group from the parent. Size = " << mParent->mChildNodes.size() << std::endl;
                mParent->mChildNodes.erase(it);
            }
        }
        return true;
    }
    else {
        // Otherwise, search the other groups to remove it.
        for (auto group: mChildNodes) {
            if (group->removeWindow(window)) {
                return true;
            }
        }
        return false;
    }
}

std::shared_ptr<TileNode> TileNode::makeTileNode(std::shared_ptr<miral::Window> window, PlacementStrategy placementStrategy) {
    // Capture the size of the window to make it the size of the new group.
    auto nextGroupPosition = window->top_left();
    auto nextGroupSize = window->size();
    auto zoneSize = mir::geometry::Rectangle(nextGroupPosition, nextGroupSize);
    auto tileNode = std::make_shared<TileNode>(zoneSize, placementStrategy);
    tileNode->mWindow = window;
    return tileNode;
}

PlacementStrategy TileNode::getPlacementStrategy() {
    return mPlacementStrategy;
}

void TileNode::setPlacementStrategy(PlacementStrategy strategy) {
    mPlacementStrategy = strategy;
}

std::shared_ptr<TileNode> TileNode::getControllingTileNode() {
    if (mPlacementStrategy == PlacementStrategy::Parent) {
        return mParent->getControllingTileNode();
    }

    return shared_from_this();
}

bool TileNode::isEmpty() {
    return mParent.get() == nullptr && mWindow.get() == nullptr && mChildNodes.size() == 0;
}

std::vector<std::shared_ptr<miral::Window>> TileNode::getWindowsInTile() {
    std::vector<std::shared_ptr<miral::Window>> retval;
    if (mWindow.get()) {
        retval.push_back(mWindow);
    }

    for (auto tileNode : mChildNodes) {
        auto otherRetval = tileNode->getWindowsInTile();
        for (auto otherWindow : otherRetval) {
            retval.push_back(otherWindow);
        }
    }

    return retval;
}

std::shared_ptr<TileNode> TileNode::getParent() {
    if (mParent.get()) return mParent;
    else return shared_from_this();
}

std::shared_ptr<TileNode> TileNode::getTileNodeForWindow(std::shared_ptr<miral::Window> window) {
    if (mWindow == window) {
        return shared_from_this();
    }
    else {
        // Otherwise, search the other groups to remove it.
        for (auto group: mChildNodes) {
            auto tileNode = group->getTileNodeForWindow(window);
            if (tileNode) {
                return tileNode;
            }
        }
        return nullptr;
    }
}