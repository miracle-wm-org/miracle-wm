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

#include "window_observer.h"

using namespace miracle;

void WindowObserverRegistrar::advise_created(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_created(container);
    }
}

void WindowObserverRegistrar::advise_closed(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_closed(container);
    }
}

void WindowObserverRegistrar::advise_window_focused(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_focused(container);
    }
}

void WindowObserverRegistrar::advise_window_fullscreen(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_fullscreen(container);
    }
}

void WindowObserverRegistrar::advise_window_move(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_move(container);
    }
}

void WindowObserverRegistrar::advise_window_float(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_float(container);
    }
}

void WindowObserverRegistrar::advise_window_marked(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_window_marked(container);
    }
}

void WindowObserverRegistrar::advise_urgency_changed(Container const& container) const
{
    for (auto const& observer : observers)
    {
        if (auto const lock = observer.lock())
            lock->on_urgency_changed(container);
    }
}
