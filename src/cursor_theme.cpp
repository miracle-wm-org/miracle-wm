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
#include "cursor_theme.h"
#include <mir/server.h>

using namespace miracle;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

CursorTheme::CursorTheme(std::string const& theme)
    : cursor_images(std::make_shared<miral::XCursorLoader>(theme))
{
}

void CursorTheme::set_cursor_theme(std::string const& theme)
{
    cursor_images->load_cursor_theme(theme);
}

void CursorTheme::operator()(mir::Server& server) const
{
    server.override_the_cursor_images([&]
    {
        return cursor_images;
    });
}