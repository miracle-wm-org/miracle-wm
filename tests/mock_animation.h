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

#ifndef MOCK_ANIMATION_H
#define MOCK_ANIMATION_H
#include "animator.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{
    class MockAnimation : public BuiltInAnimation
    {
    public:
        MockAnimation(
            AnimationHandle handle,
            float duration,
            BuiltInAnimationDefinition definition,
            mir::geometry::Rectangle const& from,
            mir::geometry::Rectangle const& to,
            mir::geometry::Rectangle const& current,
            float opacity_start,
            float opacity_end) :
            BuiltInAnimation(handle, duration, definition, from, to, current, opacity_start, opacity_end)
        {
        }

        MOCK_METHOD(void, on_tick, (AnimationFrameResult const&), (override));
    };
}
}

#endif // MOCK_ANIMATION_H
