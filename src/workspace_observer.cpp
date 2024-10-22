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

void WorkspaceObserverRegistrar::advise_created(Output const& info, int key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_created(info, key);
    }
}

void WorkspaceObserverRegistrar::advise_removed(Output const& info, int key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_removed(info, key);
    }
}

void WorkspaceObserverRegistrar::advise_focused(
    Output const* previous,
    int previous_key,
    Output const* current,
    int current_key)
{
    for (auto& observer : observers)
    {
        if (!observer.expired())
            observer.lock()->on_focused(previous, previous_key, current, current_key);
    }
}
