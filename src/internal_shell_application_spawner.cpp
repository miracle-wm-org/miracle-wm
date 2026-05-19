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

#define MIR_LOG_COMPONENT "internal_shell_application_spawner"

#include "internal_shell_application_spawner.h"
#include "parent_background_internal_client.h"
#include "parent_container.h"
#include "stacking_header_internal_client.h"
#include <mir/log.h>

using namespace miracle;

namespace
{
class NotifyingInternalApplication : public ShellApplication
{
public:
    NotifyingInternalApplication(
        std::unique_ptr<ShellApplication> application,
        std::function<void()>&& on_destroyed) :
        app(std::move(application)),
        on_destroyed(std::move(on_destroyed))
    {
    }

    void stop() override
    {
        app->stop();
    }

    miral::Application application() override
    {
        return app->application();
    }

private:
    std::unique_ptr<ShellApplication> app;
    std::function<void()> on_destroyed;
};
}

InternalShellApplicationSpawner::InternalShellApplicationSpawner(mir::Server& server) :
    server(server)
{
}

std::unique_ptr<ShellApplication> InternalShellApplicationSpawner::spawn(
    ShellApplicationRole role,
    std::shared_ptr<ShellApplicationDelegate> const& delegate)
{
    switch (role)
    {
    case ShellApplicationRole::parent_container_background:
    {
        size_t i = 0;
        for (i = 0; i < client_pool.size(); ++i)
        {
            if (!client_pool[i].is_taken)
                break;
        }

        if (i == client_pool.size())
        {
            miral::InternalClientLauncher launcher;
            launcher(server);
            client_pool.push_back({ launcher, true });
        }

        auto background_client = std::make_unique<ParentBackgroundInternalClient>();
        client_pool[i].launcher(server);
        client_pool[i].launcher.launch(*background_client);

        auto wrapper_client = std::make_unique<NotifyingInternalApplication>(
            std::move(background_client),
            [this, i]
        {
            client_pool[i].is_taken = false;
        });
        return wrapper_client;
    }
    case ShellApplicationRole::stacking_header:
    {
        auto* positioner = dynamic_cast<ParentContainer::StackingHeaderPositioner*>(delegate.get());
        if (!positioner)
        {
            mir::log_error("stacking_header role requires a StackingHeaderPositioner delegate");
            return nullptr;
        }

        auto header_client = std::make_unique<StackingHeaderInternalClient>(positioner->tab_state());

        miral::InternalClientLauncher launcher;
        launcher(server);
        launcher.launch(*header_client);

        return std::make_unique<NotifyingInternalApplication>(
            std::move(header_client),
            [] { });
    }
    }

    mir::log_error("Cannot spawn a shell application with an unknown role");
    return nullptr;
}
