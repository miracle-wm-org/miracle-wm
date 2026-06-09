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

#ifndef MIRACLEWM_ERROR_REPORTER_CONTROLLER_H
#define MIRACLEWM_ERROR_REPORTER_CONTROLLER_H

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

/// Launches the configured error reporter client whenever a configuration load
/// produces errors. The client subscribes to the `config_errors` IPC event and
/// displays the errors to the user.
///
/// If the reporter is already running, no new instance is launched; the running
/// client receives the updated errors through the IPC event instead. The user
/// closing the reporter terminates its process, after which the next erroneous
/// reload launches a fresh instance.
class ErrorReporterController : public ConfigObserver
{
public:
    ErrorReporterController(
        miral::ExternalClientLauncher& launcher,
        std::shared_ptr<Config> const& config);

    void on_config_changed(Config const& config) override;

private:
    [[nodiscard]] bool is_reporter_running() const;

    miral::ExternalClientLauncher& launcher;
    std::shared_ptr<Config> config;
    std::optional<pid_t> reporter_pid;
};
}

#endif // MIRACLEWM_ERROR_REPORTER_CONTROLLER_H
