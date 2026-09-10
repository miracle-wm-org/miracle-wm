/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MIRACLEWM_DEBUG_OVERLAY_CONTROLLER_H
#define MIRACLEWM_DEBUG_OVERLAY_CONTROLLER_H

#include "config_observer.h"
#include <memory>
#include <optional>
#include <sys/types.h>

namespace miral
{
class ExternalClientLauncher;
}

namespace miracle
{
class Config;

/// Launches and stops the configured debug overlay client on demand. The
/// overlay is a transparent, input-passthrough client that draws debugging
/// information (window geometry, clip areas, the window under the cursor, the
/// cursor position, hidden windows, ...) on top of every other window.
///
/// Unlike the error reporter, the overlay is toggled explicitly by the user via
/// `miraclemsg debug`. Toggling it on launches the client; toggling it off
/// terminates the running process. The client polls the compositor over the IPC
/// socket (IPC_GET_DEBUG_STATE) for the information it draws.
class DebugOverlayController : public ConfigObserver
{
public:
    DebugOverlayController(
        miral::ExternalClientLauncher& launcher,
        std::shared_ptr<Config> const& config);

    /// Honors a configuration change. If the overlay has been disabled while
    /// running, it is stopped.
    void on_config_changed(Config const& config) override;

    /// Toggles the overlay: launches it if it is not running, otherwise stops
    /// it. Launching is a no-op when the overlay is configured as "disabled".
    void toggle();

    /// Forces the overlay on (\p enabled == true) or off.
    void set_enabled(bool enabled);

    [[nodiscard]] bool is_running() const;

private:
    void launch();
    void stop();

    miral::ExternalClientLauncher& launcher;
    std::shared_ptr<Config> config;
    std::optional<pid_t> overlay_pid;
};
}

#endif // MIRACLEWM_DEBUG_OVERLAY_CONTROLLER_H
