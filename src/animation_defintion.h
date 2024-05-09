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

#ifndef MIRACLE_WM_ANIMATION_DEFINTION_H
#define MIRACLE_WM_ANIMATION_DEFINTION_H

#include "direction.h"
#include <string>

namespace miracle
{
/// Defines an event that can be animated.
enum class AnimateableEvent
{
    window_open,
    window_move,
    window_close,
    max
};

/// Defines the ease function for an event
enum class EaseFunction
{
    linear,
    ease_out_back,
    max
};

enum class AnimationType
{
    slide,
    grow,
    shrink,
    max
};

/// Defines an animation that the Animator can use.
/// Each animations is mapped to a single AnimateableEvent.
struct AnimationDefinition
{
    AnimationType type = AnimationType::max;
    EaseFunction function = EaseFunction::linear;
    float duration_seconds = 1.f;

    // Easing function values
    float c1 = 1.2;
    float c3 = 2.2;
};

AnimateableEvent from_string_animateable_event(std::string const&);
EaseFunction from_string_ease_function(std::string const&);
AnimationType from_string_animation_type(std::string const&);

}

#endif // MIRACLE_WM_ANIMATION_DEFINTION_H
