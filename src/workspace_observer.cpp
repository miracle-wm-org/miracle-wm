#include "workspace_observer.h"

using namespace miracle;

void WorkspaceObserverRegistrar::register_interest(std::weak_ptr<WorkspaceObserver> observer)
{
    observers.push_back(observer);
}

void WorkspaceObserverRegistrar::unregister_interest(miracle::WorkspaceObserver&)
{
    // TODO: 
}

void WorkspaceObserverRegistrar::advise_created(std::shared_ptr<Screen> const& info, int key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_created(info, key);
    }
}

void WorkspaceObserverRegistrar::advise_removed(std::shared_ptr<Screen> const& info, int key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_removed(info, key);
    }
}

void WorkspaceObserverRegistrar::advise_focused(
    std::shared_ptr<Screen> const& previous,
    int previous_key,
    std::shared_ptr<Screen> const& current,
    int current_key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_focused(previous, previous_key, current, current_key);
    }
}