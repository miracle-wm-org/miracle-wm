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
    static std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> default_animation_definitions({
        { AnimationType::built_in,
         true,
         0.2f,
         { BuiltInAnimationDefinition {
                BultInAnimationType::fade,
                EaseFunction::linear,
            } } },
        { AnimationType::built_in,
         true,
         0.25f,
         { BuiltInAnimationDefinition {
                BultInAnimationType::slide,
                EaseFunction::linear,
            } } },
        { AnimationType::built_in,
         true,
         0.3f,
         { BuiltInAnimationDefinition {
                BultInAnimationType::fade,
                EaseFunction::linear,
            } } },
        { AnimationType::built_in,
         true,
         0.25f,
         { BuiltInAnimationDefinition {
                BultInAnimationType::slide,
                EaseFunction::ease_out_sine,
            } } }
    });
}
}

#endif // ANIMATION_DEFINITION_INTERNAL_H
