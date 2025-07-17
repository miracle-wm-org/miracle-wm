/**
Copyright (C) 2024  Matthew Kosarek

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

#ifndef MIRACLEWM_AUTO_RESTARTING_LAUNCHER_H
#define MIRACLEWM_AUTO_RESTARTING_LAUNCHER_H

#include "config.h"
#include <map>
#include <mir/main_loop.h>
#include <mir/server.h>
#include <miral/external_client.h>

namespace miracle
{
class Launcher
{
public:
    virtual ~Launcher() = default;
    virtual void launch(StartupApp const&) = 0;
    virtual void kill_all() = 0;
};

class AutoRestartingLauncher : public Launcher
{
public:
    AutoRestartingLauncher(mir::Server& server, miral::ExternalClientLauncher&);
    void launch(StartupApp const&) override;
    void kill_all() override;

private:
    std::map<pid_t, StartupApp> pid_to_command_map;
    std::shared_ptr<mir::MainLoop> main_loop;
    miral::ExternalClientLauncher& launcher;
    std::mutex mutable mutex;

    void reap();
};

} // miracle

#endif // MIRACLEWM_AUTO_RESTARTING_LAUNCHER_H
