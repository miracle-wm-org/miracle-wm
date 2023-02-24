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
#include "WindowGroup.hpp"
#include "mir/geometry/forward.h"
#include "mir_toolkit/events/enums.h"
#include "miral/minimal_window_manager.h"

#include <linux/input-event-codes.h>
#include <memory>
#include <miral/application_info.h>
#include <miral/internal_client.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>
#include <miral/zone.h>

#include <linux/input.h>
#include <csignal>

using namespace miral;
using namespace miral::toolkit;

FloatingWindowManagerPolicy::FloatingWindowManagerPolicy(
    WindowManagerTools const& inTools,
    miral::InternalClientLauncher const& launcher,
    std::function<void()>& shutdown_hook) :
    miral::MinimalWindowManager(inTools),
    mRootWindowGroup()
{
    mDefaultStrategy = PlacementStrategy::Horizontal;
    shutdown_hook = [this] {  };
}

FloatingWindowManagerPolicy::~FloatingWindowManagerPolicy() = default;

bool FloatingWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event) {
    return MinimalWindowManager::handle_pointer_event(event);;
}

bool FloatingWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event) {
    return MinimalWindowManager::handle_touch_event(event);
}

void FloatingWindowManagerPolicy::advise_new_window(WindowInfo const& window_info) {
    mActiveWindowGroup = mActiveWindowGroup->addWindow(std::make_shared<Window>(window_info.window()));
    WindowSpecification modifications;
    tools.place_and_size_for_state(modifications, window_info);
    tools.modify_window(window_info.window(), modifications);
}

void FloatingWindowManagerPolicy::handle_window_ready(WindowInfo& window_info) {
    MinimalWindowManager::handle_window_ready(window_info);
    return;
}

void FloatingWindowManagerPolicy::advise_focus_gained(WindowInfo const& info) {
    MinimalWindowManager::advise_focus_gained(info);
    return;
}

bool FloatingWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    if (MinimalWindowManager::handle_keyboard_event(event)) {
        return true;
    }

    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & pModifierMask;

    if ((modifiers && mir_input_event_modifier_meta)) {
        switch (scan_code) {
            case KEY_V: {
                requestNewGroup(PlacementStrategy::Vertical);
                return true;
            }
            case KEY_H: {
                requestNewGroup(PlacementStrategy::Horizontal);
                return true;
            }
            default: {
                break;
            }
        }
    }

    return false;
}

void FloatingWindowManagerPolicy::requestNewGroup(PlacementStrategy strategy) {
    auto activeWindow = tools.active_window();

    if (!activeWindow) {
        // Nothing is selected which means nothing is added to the screen.
        mDefaultStrategy = strategy;
        return;
    }

    // TODO:
}

WindowSpecification FloatingWindowManagerPolicy::place_new_window(
    ApplicationInfo const& app_info, WindowSpecification const& request_parameters)
{
    auto parameters = MinimalWindowManager::place_new_window(app_info, request_parameters);
    bool const needs_titlebar = WindowInfo::needs_titlebar(parameters.type().value());

    // If it is our first time adding an item to the view, we initialize the root window group.
    if (mRootWindowGroup.isEmpty()) {
        mRootWindowGroup = WindowGroup(tools.active_application_zone().extents(), mDefaultStrategy);
        mActiveWindowGroup = std::make_shared<WindowGroup>(mRootWindowGroup);
    }

    auto targetNumberOfWindows = mActiveWindowGroup->getNumTilesInGroup() + 1;
    auto activeZone = mActiveWindowGroup->getZone();
    auto placementStrategy = mActiveWindowGroup->getPlacementStrategy();

    if (targetNumberOfWindows == 1) {
        // There are no windows in the current zone so we can place it to take up the whole zone.
        parameters.top_left() = activeZone.extents().top_left;
        parameters.size() = Size{ activeZone.extents().size };
    }
    else if (placementStrategy == PlacementStrategy::Horizontal) {
        auto zoneFractionSize = Size{ activeZone.extents().size.width / targetNumberOfWindows, activeZone.extents().size.height };
        const int y = activeZone.extents().top_left.y.as_value();
        for (unsigned short i = 0; auto window : mActiveWindowGroup->getWindowsInZone()) {
            window->resize(zoneFractionSize);

            const int x = zoneFractionSize.width.as_int() * i + activeZone.extents().top_left.x.as_value();
            window->move_to(Point{ x, y });
            i++;
        }

        const int x = zoneFractionSize.width.as_int() * mActiveWindowGroup->getNumTilesInGroup() + activeZone.extents().top_left.x.as_value();
        parameters.top_left() = Point{ x, y };
        parameters.size() = zoneFractionSize;
    }
    else if (placementStrategy == PlacementStrategy::Vertical) {
        auto zoneFractionSize = Size{ activeZone.extents().size.width, activeZone.extents().size.height / targetNumberOfWindows };
        const int x = activeZone.extents().top_left.x.as_value();
        for (unsigned short i = 0; auto window : mActiveWindowGroup->getWindowsInZone()) {
            window->resize(zoneFractionSize);

            const int y = zoneFractionSize.height.as_int() * i + activeZone.extents().top_left.y.as_value();
            window->move_to(Point{ x, y });
            i++;
        }

        const int y = zoneFractionSize.height.as_int() * mActiveWindowGroup->getNumTilesInGroup() + activeZone.extents().top_left.y.as_value();
        parameters.top_left() = Point{ x, y };
        parameters.size() = zoneFractionSize;
    }

    return parameters;
}

void FloatingWindowManagerPolicy::handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) {
    MinimalWindowManager::handle_modify_window(window_info, modifications);
    return;
}
