
#ifndef MIRACLE_WM_SHELL_APPLICATION_MANAGER_H
#define MIRACLE_WM_SHELL_APPLICATION_MANAGER_H

#include <miral/application.h>
#include <vector>

namespace miracle
{
enum class ShellApplicationType
{
    parent_container_background
};

/// Manages applications that have special shell roles, such as being
/// rendered in the background of parent containers.
///
/// Miracle will use this to specially position such applications.
class ShellApplicationManager
{
public:
    void register_app(miral::Application const& application, ShellApplicationType type);
    void unregister_app(miral::Application const& application, ShellApplicationType type);
    bool is_registered(miral::Application const& application, ShellApplicationType type) const;

private:
    std::vector<miral::Application> parent_container_background_apps;
};
}

#endif // MIRACLE_WM_SHELL_APPLICATION_MANAGER_H