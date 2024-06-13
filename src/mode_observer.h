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

#ifndef MIRACLEWM_MODE_OBSERVER_H
#define MIRACLEWM_MODE_OBSERVER_H

#include "compositor_state.h"
#include "observer_registrar.h"

namespace miracle
{

class ModeObserver
{
public:
    virtual ~ModeObserver() = default;
    virtual void on_changed(WindowManagerMode mode) = 0;
};

class ModeObserverRegistrar : public ObserverRegistrar<ModeObserver>
{
public:
    ModeObserverRegistrar() = default;
    void advise_changed(WindowManagerMode mode);
};

}

#endif
