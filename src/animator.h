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

#ifndef MIRACLEWM_ANIMATOR_H
#define MIRACLEWM_ANIMATOR_H

#include <miral/window.h>

namespace miracle
{

class Animator
{
public:
    /// This will take the MainLoop to schedule animation.
    explicit Animator();
    ~Animator();

    /// Queue an animation on a window from the provided point to the other
    /// point. It is assumed that the window has already been moved to the
    /// "to" position.
    void animate_window_movement(
        miral::Window const&,
        mir::geometry::Point const& from,
        mir::geometry::Point const& to);
};

} // miracle

#endif //MIRACLEWM_ANIMATOR_H
