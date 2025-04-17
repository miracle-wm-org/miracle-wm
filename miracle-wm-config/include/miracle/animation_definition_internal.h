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

#ifndef ANIMATION_DEFINITION_INTERNAL_H
#define ANIMATION_DEFINITION_INTERNAL_H

#include "animation_definition.h"

namespace miracle
{
namespace internal
{
    std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> constexpr default_animation_definitions({
        {
         true,
         AnimationType::fade_in,
         EaseFunction::linear,
         0.3f,
         },
        {
         true,
         AnimationType::slide,
         EaseFunction::linear,
         0.25f,
         },
        {
         true,
         AnimationType::fade_out,
         EaseFunction::linear,
         0.3f,
         },
        { true,
         AnimationType::slide,
         EaseFunction::ease_out_sine,
         0.25f }
    });
}
}

#endif // ANIMATION_DEFINITION_INTERNAL_H
