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

#ifndef MIRACLE_SYNCHRONISED_RECURSIVE_H_
#define MIRACLE_SYNCHRONISED_RECURSIVE_H_

#include <mutex>

namespace miracle
{

/**
 * An object that enforces unique access to the data it contains
 *
 * This behaves like a mutex which owns the data it guards, and
 * can give out a smart-pointer-esque handle to lock and access it.
 *
 * \tparam T  The type of data contained
 */
template <typename T>
class SynchronisedRecursive
{
public:
    SynchronisedRecursive() = default;
    SynchronisedRecursive(T&& initial_value) :
        value { std::move(initial_value) }
    {
    }

    SynchronisedRecursive(SynchronisedRecursive const&) = delete;
    SynchronisedRecursive& operator=(SynchronisedRecursive const&) = delete;

    /**
     * Smart-pointer-esque accessor for the protected data.
     *
     * Ensures exclusive access to the referenced data.
     *
     * \note Instances of Locked must not outlive the SynchronisedRecursive
     *       they are derived from.
     */
    template <typename U>
    class LockedImpl
    {
    public:
        LockedImpl(LockedImpl&& from) noexcept
            :
            value { from.value },
            lock { std::move(from.lock) }
        {
            from.value = nullptr;
        }

        ~LockedImpl() = default;

        auto operator*() const -> U&
        {
            return *value;
        }

        auto operator->() const -> U*
        {
            return value;
        }

        /**
         * Relinquish access to the data
         *
         * This prevents further access to the contained data through
         * this handle, and allows other code to acquire access.
         */
        void drop()
        {
            value = nullptr;
            lock.unlock();
        }

        /**
         * Allows waiting for a condition variable
         *
         * The protected data may be accessed both in the predicate and after this method completes.
         */
        template <typename Cv, typename Predicate>
        void wait(Cv& cv, Predicate stop_waiting)
        {
            cv.wait(lock, stop_waiting);
        }

    private:
        friend class SynchronisedRecursive;
        LockedImpl(std::unique_lock<std::recursive_mutex>&& lock, U& value) :
            value { &value },
            lock { std::move(lock) }
        {
        }

        U* value;
        std::unique_lock<std::recursive_mutex> lock;
    };

    /**
     * Smart-pointer-esque accessor for the protected data.
     *
     * Ensures exclusive access to the referenced data.
     *
     * \note Instances of Locked must not outlive the SynchronisedRecursive
     *       they are derived from.
     */
    using Locked = LockedImpl<T>;
    /**
     * Lock the data and return an accessor.
     *
     * \return A smart-pointer-esque accessor for the contained data.
     *         While code has access to a live Locked instance it is
     *         guaranteed to have unique access to the contained data.
     */
    auto lock() -> Locked
    {
        return LockedImpl<T> { std::unique_lock { mutex }, value };
    }

    /**
     * Smart-pointer-esque accessor for the protected data.
     *
     * Provides const access to the protected data, with the guarantee
     * that no changes to the data can be made while this handle
     * is live.
     *
     * \note Instances of Locked must not outlive the SynchronisedRecursive
     *       they are derived from.
     */
    using LockedView = LockedImpl<T const>;
    /**
     * Lock the data and return an accessor.
     *
     * \return A smart-pointer-esque accessor for the contained data.
     *         While code has access to a live Locked instance it is
     *         guaranteed to have unique access to the contained data.
     */
    auto lock() const -> LockedView
    {
        return LockedImpl<T const> { std::unique_lock { mutex }, value };
    }

private:
    std::recursive_mutex mutable mutex;
    T value;
};

}

#endif // MIRACLE_SYNCHRONISED_RECURSIVE_H_
