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

#ifndef MIRACLEWM_OBSERVER_REGISTRAR_H
#define MIRACLEWM_OBSERVER_REGISTRAR_H

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace miracle
{
template <typename T>
class ObserverRegistrar
{
public:
    virtual ~ObserverRegistrar() = default;
    void register_interest(std::weak_ptr<T> observer)
    {
        std::lock_guard lock(mutex);
        observers.push_back(observer);
    }

    void unregister_interest(T* observer)
    {
        std::lock_guard lock(mutex);
        observers.erase(std::remove_if(observers.begin(), observers.end(), [&observer](std::weak_ptr<T> const& other)
        {
            if (other.expired())
                return true;

            return other.lock().get() == observer;
        }));
    }

    void for_each_observer(std::function<void(T*)> const& f)
    {
        std::vector<std::weak_ptr<T>> observers_copy;
        {
            std::lock_guard lock(mutex);
            observers_copy = observers;
        }
        for (auto const& observer : observers_copy)
        {
            if (auto ptr = observer.lock())
                f(ptr.get());
        }
    }

protected:
    std::mutex mutex;
    std::vector<std::weak_ptr<T>> observers;
};
}

#endif
