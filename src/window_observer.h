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

#ifndef WINDOW_OBSERVER_H
#define WINDOW_OBSERVER_H

#include "observer_registrar.h"

namespace miracle
{
class Container;

/// This observer implements a general interface for:
/// https://i3wm.org/docs/ipc.html#_window_event
class WindowObserver
{
public:
    virtual ~WindowObserver() = default;
    virtual void on_window_created(Container const&) = 0;
    virtual void on_window_closed(Container const&) = 0;
    virtual void on_window_focused(Container const&) = 0;
    virtual void on_window_fullscreen(Container const&) = 0;
    virtual void on_window_move(Container const&) = 0;
    virtual void on_window_float(Container const&) = 0;
    virtual void on_window_marked(Container const&) = 0;
};

class NullWindowObserver : public WindowObserver
{
public:
    void on_window_created(Container const&) override { }
    void on_window_closed(Container const&) override { }
    void on_window_focused(Container const&) override { }
    void on_window_fullscreen(Container const&) override { }
    void on_window_move(Container const&) override { }
    void on_window_float(Container const&) override { }
    void on_window_marked(Container const&) override { }
};

class WindowObserverRegistrar : public ObserverRegistrar<WindowObserver>
{
public:
    void advise_created(Container const&) const;
    void advise_closed(Container const&) const;
    void advise_window_focused(Container const&) const;
    void advise_window_fullscreen(Container const&) const;
    void advise_window_move(Container const&) const;
    void advise_window_float(Container const&) const;
    void advise_window_marked(Container const&) const;
};

}

#endif // WINDOW_OBSERVER_H
