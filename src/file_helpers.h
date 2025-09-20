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

#ifndef MIRACLE_FILE_HELPERS_H
#define MIRACLE_FILE_HELPERS_H
#include <string>

namespace miracle
{
inline std::string expand_tilde_getenv(const std::string& path)
{
    if (path.empty() || path[0] != '~')
    {
        return path;
    }

    const char* home_dir = std::getenv("HOME");
    if (home_dir == nullptr)
    {
        return path;
    }

    std::string expanded_path = home_dir;
    if (path.length() > 1 && path[1] == '/')
    {
        expanded_path += path.substr(1);
    }
    else if (path.length() > 1 && path[1] != '/')
    {
        return path;
    }
    return expanded_path;
}
}
#endif
