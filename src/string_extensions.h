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

#ifndef MIRACLE_WM_STRING_EXTENSIONS_H
#define MIRACLE_WM_STRING_EXTENSIONS_H

#include <string_view>

template <typename T>
std::string_view trim_left(T const& data)
{
    std::string_view sv { data };
    sv.remove_prefix(std::min(sv.find_first_not_of(' '), sv.size()));
    return sv;
}

#endif // MIRACLE_WM_STRING_EXTENSIONS_H
