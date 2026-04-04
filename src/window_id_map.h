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

#ifndef MIRACLE_WM_WINDOW_ID_MAP_H
#define MIRACLE_WM_WINDOW_ID_MAP_H

#include <cstdint>
#include <miral/window.h>
#include <unordered_map>

namespace miracle
{
using WindowIdMap = std::unordered_map<uint64_t, miral::Window>;
}

#endif // MIRACLE_WM_WINDOW_ID_MAP_H
