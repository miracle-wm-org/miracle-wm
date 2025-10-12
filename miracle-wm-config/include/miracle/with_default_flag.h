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

#ifndef MIRACLE_DEFAULT_TRACKER_H
#define MIRACLE_DEFAULT_TRACKER_H
#include <type_traits>
#include <utility>

namespace miracle
{
template <typename T>
class WithDefaultFlag
{
public:
    WithDefaultFlag() = default;
    WithDefaultFlag(const T& v) :
        value(v),
        is_default_value(true)
    {
    }
    WithDefaultFlag(const WithDefaultFlag&) = default;
    WithDefaultFlag(WithDefaultFlag&&) noexcept = default;
    WithDefaultFlag& operator=(const WithDefaultFlag&) = default;
    WithDefaultFlag& operator=(WithDefaultFlag&&) noexcept = default;

    WithDefaultFlag& operator=(const T& v)
    {
        value = v;
        is_default_value = false;
        return *this;
    }

    WithDefaultFlag& operator=(T&& v)
    {
        value = std::move(v);
        is_default_value = false;
        return *this;
    }

    operator T&() = delete;
    operator const T&() const { return value; }

    T* operator->()
    {
        is_default_value = false;
        return &value;
    }
    const T* operator->() const { return &value; }

    T& operator*() { return value; }
    const T& operator*() const { return value; }

    bool operator==(const WithDefaultFlag& other) const = delete;
    bool operator!=(const WithDefaultFlag& other) const = delete;

    T value {};
    bool is_default_value { true };
};
}

#endif
