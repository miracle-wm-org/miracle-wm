#include "workspace_observer.h"

using namespace miracle;

namespace
{
static int NEXT_ID = 0;
}

WorkspaceObserver::WorkspaceObserver()
    : id{NEXT_ID}
{}

int WorkspaceObserver::get_id() const
{
    return id;
}

void WorkspaceObserverRegistrar::register_interest(std::weak_ptr<WorkspaceObserver> observer)
{
    observers.push_back(observer);
}

void WorkspaceObserverRegistrar::unregister_interest(miracle::WorkspaceObserver& observer)
{
    observers.erase(std::remove_if(observers.begin(), observers.end(), [&observer](std::weak_ptr<WorkspaceObserver> const& other)
    {
        if (other.expired())
            return true;

        return other.lock()->get_id() == observer.get_id();
    }));
}

void WorkspaceObserverRegistrar::advise_created(std::shared_ptr<OutputContent> const& info, int key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_created(info, key);
    }
}

void WorkspaceObserverRegistrar::advise_removed(std::shared_ptr<OutputContent> const& info, int key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_removed(info, key);
    }
}

void WorkspaceObserverRegistrar::advise_focused(
    std::shared_ptr<OutputContent> const& previous,
    int previous_key,
    std::shared_ptr<OutputContent> const& current,
    int current_key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_focused(previous, previous_key, current, current_key);
    }
}