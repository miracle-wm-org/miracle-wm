/**
Copyright (C) 2026  Matthew Kosarek

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

#ifndef MIRACLEWM_SESSION_LOCK_OBSERVER_H
#define MIRACLEWM_SESSION_LOCK_OBSERVER_H

#include "observer_registrar.h"

namespace miracle
{

/// Notified when the session is locked or unlocked (e.g. by a client of
/// ext-session-lock-v1).
///
/// Notifications arrive on whichever thread Mir changed the lock state on,
/// which is not the window management thread.
class SessionLockObserver
{
public:
    virtual ~SessionLockObserver() = default;
    virtual void on_session_locked() = 0;
    virtual void on_session_unlocked() = 0;
};

class SessionLockObserverRegistrar : public ObserverRegistrar<SessionLockObserver>
{
public:
    SessionLockObserverRegistrar() = default;
    void advise_locked();
    void advise_unlocked();
};

}

#endif
