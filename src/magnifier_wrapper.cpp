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

#include "magnifier_wrapper.h"

using namespace miracle;

MagnifierWrapper::MagnifierWrapper(miral::Magnifier const& magnifier) :
    magnifier(magnifier)
{
}

bool MagnifierWrapper::enable()
{
    if (enabled)
        return false;

    magnifier.enable(true);
    enabled = true;
    return true;
}

bool MagnifierWrapper::disable()
{
    if (!enabled)
        return false;

    magnifier.enable(false);
    enabled = false;
    has_changed = false;
    return true;
}

void MagnifierWrapper::set_scale(float new_scale)
{
    magnifier.magnification(new_scale);
    scale = new_scale;
    has_changed = true;
}

void MagnifierWrapper::set_size(int new_width, int new_height)
{
    magnifier.capture_size(mir::geometry::Size(new_width, new_height));
    width = new_width;
    height = new_height;
    has_changed = true;
}
