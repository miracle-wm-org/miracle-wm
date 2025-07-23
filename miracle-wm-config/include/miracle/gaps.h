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

#ifndef MIRACLEWM_GAPS_H
#define MIRACLEWM_GAPS_H

#include <cstddef>

namespace miracle
{
struct Gaps
{
    size_t top = 0;
    size_t bottom = 0;
    size_t left = 0;
    size_t right = 0;
};

inline Gaps operator+(Gaps const& lhs, Gaps const& rhs)
{
    Gaps result;
    result.top = lhs.top + rhs.top;
    result.bottom = lhs.bottom + rhs.bottom;
    result.left = lhs.left + rhs.left;
    result.right = lhs.right + rhs.right;
    return result;
}

inline Gaps operator-(Gaps const& lhs, Gaps const& rhs)
{
    Gaps result;
    result.top = lhs.top - rhs.top;
    result.bottom = lhs.bottom - rhs.bottom;
    result.left = lhs.left - rhs.left;
    result.right = lhs.right - rhs.right;
    return result;
}

inline bool operator==(Gaps const& lhs, Gaps const& rhs)
{
    return lhs.top == rhs.top && lhs.bottom == rhs.bottom && lhs.left == rhs.left && lhs.right == rhs.right;
}
}

#endif // MIRACLEWM_GAPS_H
