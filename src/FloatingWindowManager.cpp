/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FloatingWindowManager.hpp"
#include "mir/geometry/forward.h"
#include "mir/logging/logger.h"
#include "mir_toolkit/events/enums.h"
#include "miral/application.h"
#include "miral/minimal_window_manager.h"

#include <linux/input-event-codes.h>
#include <memory>
#include <miral/application_info.h>
#include <miral/internal_client.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>
#include <miral/zone.h>
#include <iostream>

#include <linux/input.h>
#include <csignal>
#include <ostream>
#include <stdexcept>
#include <utility>

using namespace miral;
using namespace miral::toolkit;

FloatingWindowManagerPolicy::FloatingWindowManagerPolicy(
    WindowManagerTools const& inTools,
    miral::InternalClientLauncher const& launcher,
    std::function<void()>& shutdown_hook) :
    miral::MinimalWindowManager(inTools),
    mRootTileNode()
{
    mDefaultStrategy = PlacementStrategy::Horizontal;
    shutdown_hook = [this] {  };
    mActiveTileNode = nullptr;
}

FloatingWindowManagerPolicy::~FloatingWindowManagerPolicy() = default;

bool FloatingWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event) {
    if (MinimalWindowManager::handle_keyboard_event(event)) {
        return true;
    }

    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & pModifierMask;

    if (action == MirKeyboardAction::mir_keyboard_action_down && (modifiers && mir_input_event_modifier_meta)) {
        if (scan_code == KEY_V) {
            requestPlacementStrategyChange(PlacementStrategy::Vertical);
            return true;
        }
        else if (scan_code == KEY_H) {
            requestPlacementStrategyChange(PlacementStrategy::Horizontal);
            return true;
        }
        else if (scan_code == KEY_LEFT) {
            requestChangeActiveWindow(-1, mActiveTileNode);
            return true;
        }
        else if (scan_code == KEY_RIGHT) {
            requestChangeActiveWindow(1, mActiveTileNode);
            return true;
        }
        else if ((modifiers && mir_input_event_modifier_shift) && scan_code == KEY_Q) {
            requestQuitSelectedApplication();
            return true;
        }
    }

    return false;
}

void FloatingWindowManagerPolicy::requestPlacementStrategyChange(PlacementStrategy strategy) {
    auto activeWindow = tools.active_window();

    if (!activeWindow) {
        // Nothing is selected which means nothing is added to the screen.
        mDefaultStrategy = strategy;
        return;
    }

    mActiveTileNode->setPlacementStrategy(strategy);
}

void FloatingWindowManagerPolicy::requestQuitSelectedApplication() {
    if (!mActiveWindow.get()) {
        std::cout << "There is no current application to quit." << std::endl;
        return;
    }

    auto application = mActiveWindow->application();
    mRootTileNode->removeWindow(mActiveWindow);
    miral::kill(application, SIGTERM);

    // TODO: If I had more time, I would resize the grid when a quit occurs.

    // Select the next available window.
    mActiveTileNode = mActiveTileNode->getParent();
    if (mActiveTileNode->getWindowsInTile().size()) {
        mActiveWindow = mActiveTileNode->getWindowsInTile().at(0);
        tools.select_active_window(*mActiveWindow.get());
        mActiveTileNode = mRootTileNode->getTileNodeForWindow(mActiveWindow);
    }
    else {
        mActiveWindow.reset();
    }

    std::cout << "Quit the current application." << std::endl;
}

bool FloatingWindowManagerPolicy::requestChangeActiveWindow(int moveAmount, std::shared_ptr<TileNode> tileNodeToSearch) {
    auto parent = tileNodeToSearch->getParent();
    if (!parent.get()) {
        // We are a root node, all moves are illegal.
        std::cout << "Cannot change active window because we are at the root" << std::endl;
        return false;
    }

    auto parentsChildren = parent->getSubTileNodes();
    auto it = std::find(parentsChildren.begin(), parentsChildren.end(), tileNodeToSearch);
    if (it == parentsChildren.end())  {
        std::cerr << "Could not change the active window because the window group is not found." << std::endl;
        return false;
    }

    int index = it - parentsChildren.begin();
    int newIndex = index + moveAmount;
    if (newIndex < parentsChildren.size() && newIndex >= 0) {
        // We can make a lateral move to another node in the tree
        auto nextTileNode = parentsChildren.at(newIndex);
        if (nextTileNode->getWindowsInTile().empty()) {
            std::cerr << "Encountered error state: Found window group without any windows." << std::endl;
        }

        mActiveTileNode = nextTileNode;
        mActiveWindow = mActiveTileNode->getWindowsInTile().at(0);
        tools.select_active_window(*mActiveWindow.get());
    }
    else if (index > 0) {
        // We're going to try to go deeper into the tree
        for (auto child : tileNodeToSearch->getSubTileNodes()) {
            if (requestChangeActiveWindow(moveAmount, child)) {
                return true;
            }
        }

        return false;
    }
    else if (parent.get()) {
        // We're going to try and go higher into the tree
        return requestChangeActiveWindow(moveAmount, parent);
    }
}

WindowSpecification FloatingWindowManagerPolicy::place_new_window(
    ApplicationInfo const& app_info, WindowSpecification const& request_parameters)
{
    auto parameters = MinimalWindowManager::place_new_window(app_info, request_parameters);

    // If it is our first time adding an item to the view, we initialize the root window group.
    if (!mRootTileNode.get()) {
        mRootTileNode = std::make_shared<TileNode>(tools.active_application_zone().extents(), mDefaultStrategy);
        mActiveTileNode = mRootTileNode;
    }

    auto groupInCharge = mActiveTileNode->getControllingTileNode();
    auto targetNumberOfWindows = groupInCharge->getNumberOfTiles() + 1;
    auto activeZone = groupInCharge->getZone();
    auto placementStrategy = groupInCharge->getPlacementStrategy();

    std::cout << "Placing new window into group. Group ID: " << activeZone.id() << ". Target Number of Windows: " << targetNumberOfWindows << std::endl;
    if (targetNumberOfWindows == 1) {
        
        // There are no windows in the current zone so we can place it to take up the whole zone.
        parameters.top_left() = activeZone.extents().top_left;
        parameters.size() = Size{ activeZone.extents().size };
    }
    else if (placementStrategy == PlacementStrategy::Horizontal) {
        auto zoneFractionSize = Size{ activeZone.extents().size.width / targetNumberOfWindows, activeZone.extents().size.height };
        const int y = activeZone.extents().top_left.y.as_value();

        for (unsigned short i = 0; auto window : groupInCharge->getWindowsInTile()) {
            window->resize(zoneFractionSize);

            const int x = zoneFractionSize.width.as_int() * i + activeZone.extents().top_left.x.as_value();
            window->move_to(Point{ x, y });
            i++;
        }

        const int x = zoneFractionSize.width.as_int() * groupInCharge->getNumberOfTiles() + activeZone.extents().top_left.x.as_value();
        parameters.top_left() = Point{ x, y };
        parameters.size() = zoneFractionSize;
    }
    else if (placementStrategy == PlacementStrategy::Vertical) {
        auto zoneFractionSize = Size{ activeZone.extents().size.width, activeZone.extents().size.height / targetNumberOfWindows };
        const int x = activeZone.extents().top_left.x.as_value();
        for (unsigned short i = 0; auto window : groupInCharge->getWindowsInTile()) {
            window->resize(zoneFractionSize);

            const int y = zoneFractionSize.height.as_int() * i + activeZone.extents().top_left.y.as_value();
            window->move_to(Point{ x, y });
            i++;
        }

        const int y = zoneFractionSize.height.as_int() * groupInCharge->getNumberOfTiles() + activeZone.extents().top_left.y.as_value();
        parameters.top_left() = Point{ x, y };
        parameters.size() = zoneFractionSize;
    }

    std::cout << "Placement of window complete." << std::endl;

    return parameters;
}

bool FloatingWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event) {
    return true;
}

bool FloatingWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event) {
    return true;
}

void FloatingWindowManagerPolicy::advise_new_window(WindowInfo const& window_info) {
    std::cout << "Adding window into the TileNode" << std::endl;
    
    // Add the window into the current tile. 
    auto window = std::make_shared<Window>(window_info.window());
    mActiveTileNode = mActiveTileNode->addWindow(window);
    mActiveWindow = window;

    // Map the application to the window group so that we can remove it when we delete it.
    pid_t applicationPid = miral::pid_of(window->application());

    // Do the regular placing and sizing.
    WindowSpecification modifications;
    tools.place_and_size_for_state(modifications, window_info);
    tools.modify_window(window_info.window(), modifications);
    std::cout << "New window group has ID: " << mActiveTileNode->getZoneId() << ". New zone is now active." << std::endl;
    if (mActiveTileNode->getParent().get()) {
        std::cout << "Parent window group has ID: " << mActiveTileNode->getParent()->getZoneId() << std::endl;
    }
}

void FloatingWindowManagerPolicy::handle_window_ready(WindowInfo& window_info) {
    MinimalWindowManager::handle_window_ready(window_info);
    return;
}

void FloatingWindowManagerPolicy::advise_focus_gained(WindowInfo const& info) {
    MinimalWindowManager::advise_focus_gained(info);
    return;
}

void FloatingWindowManagerPolicy::handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) {
    MinimalWindowManager::handle_modify_window(window_info, modifications);
    return;
}
