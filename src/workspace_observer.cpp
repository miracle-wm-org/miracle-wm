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