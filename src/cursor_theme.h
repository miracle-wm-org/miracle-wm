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

#ifndef MIRACLE_WM_CURSORTHEME_H
#define MIRACLE_WM_CURSORTHEME_H

#include <string>
#include "xcursor_loader.h"


namespace mir
{
class Server;
namespace input
{
}
}

namespace miracle
{
class CursorTheme
{
public:
    explicit CursorTheme(std::string const& theme);
    void set_cursor_theme(std::string const& theme);
    void operator()(mir::Server& server) const;

private:
    std::shared_ptr<miral::XCursorLoader> cursor_images;
};
}

#endif //MIRACLE_WM_CURSORTHEME_H
