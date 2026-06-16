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

#ifndef MIRACLE_WM_CONFIG_ANIMATION_DEFINITION_H
#define MIRACLE_WM_CONFIG_ANIMATION_DEFINITION_H

#include "export.h"
#include <array>
#include <string>
#include <vector>

namespace miracle
{

/// Defines the ease function for an event
enum class MIRACLE_WM_CONFIG_API EaseFunction
{
    linear,
    ease_in_sine,
    ease_out_sine,
    ease_in_out_sine,
    ease_in_quad,
    ease_out_quad,
    ease_in_out_quad,
    ease_in_cubic,
    ease_out_cubic,
    ease_in_out_cubic,
    ease_in_quart,
    ease_out_quart,
    ease_in_out_quart,
    ease_in_quint,
    ease_out_quint,
    ease_in_out_quint,
    ease_in_expo,
    ease_out_expo,
    ease_in_out_expo,
    ease_in_circ,
    ease_out_circ,
    ease_in_out_circ,
    ease_in_back,
    ease_out_back,
    ease_in_out_back,
    ease_in_elastic,
    ease_out_elastic,
    ease_in_out_elastic,
    ease_in_bounce,
    ease_out_bounce,
    ease_in_out_bounce,
    max
};

constexpr std::array<const char*, static_cast<int>(EaseFunction::max)> ease_function_strings = {
    "linear",
    "ease_in_sine",
    "ease_out_sine",
    "ease_in_out_sine",
    "ease_in_quad",
    "ease_out_quad",
    "ease_in_out_quad",
    "ease_in_cubic",
    "ease_out_cubic",
    "ease_in_out_cubic",
    "ease_in_quart",
    "ease_out_quart",
    "ease_in_out_quart",
    "ease_in_quint",
    "ease_out_quint",
    "ease_in_out_quint",
    "ease_in_expo",
    "ease_out_expo",
    "ease_in_out_expo",
    "ease_in_circ",
    "ease_out_circ",
    "ease_in_out_circ",
    "ease_in_back",
    "ease_out_back",
    "ease_in_out_back",
    "ease_in_elastic",
    "ease_out_elastic",
    "ease_in_out_elastic",
    "ease_in_bounce",
    "ease_out_bounce",
    "ease_in_out_bounce"
};

enum class MIRACLE_WM_CONFIG_API BultInAnimationType
{
    disabled,
    slide,
    grow,
    shrink,
    fade,
    max
};

constexpr std::array<const char*, static_cast<int>(BultInAnimationType::max)> built_in_animation_type_strings = {
    "disabled",
    "slide",
    "grow",
    "shrink",
    "fade"
};

/// Defines a built-in animation for miracle.
struct MIRACLE_WM_CONFIG_API BuiltInAnimationDefinition
{
    BultInAnimationType type = BultInAnimationType::max;
    EaseFunction function = EaseFunction::linear;

    // Easing function values
    float c1 = 1.2f;
    float c2 = 1.83f;
    float c3 = 2.2f;
    float c4 = 2.0943951023931953f;
    float c5 = 1.3962634015954636f;
    float n1 = 7.5625f;
    float d1 = 2.75f;
};

typedef std::vector<BuiltInAnimationDefinition> BuiltInAnimationList;

/// Defines an animation fed to miracle's animation system.
///
/// If a plugin handles the animation event, its result is used.
/// Otherwise, the built-in animation defined by #data is used.
struct MIRACLE_WM_CONFIG_API AnimationDefinition
{
    bool is_default = true;
    float duration_seconds = 1.f;

    BuiltInAnimationList data;
};

/// Defines an event that can be animated.
enum class MIRACLE_WM_CONFIG_API AnimateableEvent
{
    window_open,
    window_move,
    window_close,
    workspace_switch,
    shell_window_open,
    shell_window_close,
    max
};

constexpr std::array<const char*, static_cast<int>(AnimateableEvent::max)> animateable_event_strings = {
    "window_open",
    "window_move",
    "window_close",
    "workspace_switch",
    "shell_window_open",
    "shell_window_close"
};
}

#endif // MIRACLE_WM_CONFIG_ANIMATION_DEFINITION_H
