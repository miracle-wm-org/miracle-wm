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

#ifndef MIRACLE_WM_STRING_EXTENSIONS_H
#define MIRACLE_WM_STRING_EXTENSIONS_H

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

static void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char const ch)
    { return !std::isspace(ch); }));
}

// Trim from end (in place)
static void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                [](unsigned char const ch)
    { return !std::isspace(ch); })
                .base(),
        s.end());
}

// Trim from both ends (in place)
static void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}
#endif // MIRACLE_WM_STRING_EXTENSIONS_H
