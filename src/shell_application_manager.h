
#ifndef MIRACLE_WM_SHELL_APPLICATION_MANAGER_H
#define MIRACLE_WM_SHELL_APPLICATION_MANAGER_H

#include <miral/application.h>
#include <vector>

namespace miracle
{
class Container;

enum class ShellApplicationType
{
    parent_container_background
};

/// Used to notify delegates about shell component events.
class ShellComponentDelegate
{
public:
    ~ShellComponentDelegate() = default;
    virtual void handle_ready(std::shared_ptr<Container> const&) = 0;
};

/// Manages applications that have special shell roles, such as being
/// rendered in the background of parent containers.
///
/// Miracle will use this to specially position such applications.
class ShellApplicationManager
{
public:
    /// Register an application with a specific shell role.
    void register_app(miral::Application const& application, ShellApplicationType type, std::shared_ptr<ShellComponentDelegate> const& delegate);

    /// Unregister an applicationn.
    void unregister_app(miral::Application const& application);

    /// Check if an application is registered.
    bool is_registered(miral::Application const& application) const;

    /// Get a delegate for the given application, or `nullptr` if none exists.
    std::shared_ptr<ShellComponentDelegate> delegate(miral::Application const& application) const;

private:
    struct ApplicationData
    {
        ShellApplicationType type;
        miral::Application application;
        std::shared_ptr<ShellComponentDelegate> delegate;
    };

    std::vector<ApplicationData> registered_apps;
};
}


#endif // MIRACLE_WM_SHELL_APPLICATION_MANAGER_H