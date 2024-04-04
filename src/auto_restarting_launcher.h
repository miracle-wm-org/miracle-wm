#ifndef MIRACLEWM_AUTO_RESTARTING_LAUNCHER_H
#define MIRACLEWM_AUTO_RESTARTING_LAUNCHER_H

#include "miracle_config.h"
#include <miral/external_client.h>
#include <miral/runner.h>
#include <string>
#include <map>

namespace miracle
{

class AutoRestartingLauncher
{
public:
    AutoRestartingLauncher(miral::MirRunner&, miral::ExternalClientLauncher&);
    void launch(miracle::StartupApp const&);
    void kill_all();

private:
    std::map<pid_t, miracle::StartupApp> pid_to_command_map;
    miral::MirRunner& runner;
    miral::ExternalClientLauncher& launcher;
    std::mutex mutable mutex;

    void reap();
};

} // miracle

#endif //MIRACLEWM_AUTO_RESTARTING_LAUNCHER_H
