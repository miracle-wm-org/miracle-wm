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

#ifndef MIRACLE_MAGNIFIER_WRAPPER_H
#define MIRACLE_MAGNIFIER_WRAPPER_H

#include <miral/magnifier.h>

namespace miracle
{
/// Wraps a #miral::Magnifier.
///
/// This class manages caching around enablement, size change, magnification,
/// and more.
class MagnifierWrapper
{
public:
    explicit MagnifierWrapper(miral::Magnifier const& magnifier);
    bool enable();
    bool disable();
    void set_scale(float scale);
    void set_size(int width, int height);
    int get_width() const { return width; }
    int get_height() const { return height; }
    float get_scale() const { return scale; }

private:
    miral::Magnifier magnifier;
    bool enabled = false;
    bool has_changed = false;
    float scale = 0;
    int width = 400;
    int height = 300;
};
}

#endif