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

#ifndef MIRACLE_WM_ANIMATION_H
#define MIRACLE_WM_ANIMATION_H

#include "plugin_manager.h"
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miracle/cpp/animation_definition.h>
#include <optional>

namespace miracle
{
class PluginManager;

/// Unique handle provided to track animators
typedef uint32_t AnimationHandle;

struct AnimationData
{
    AnimateableEvent event;
    mir::geometry::Rectangle area_start;
    mir::geometry::Rectangle area_end;
    float opacity_start;
    float opacity_end;
};

/// Data provided when the animation ticks.
///
/// Individual [Animation]s can decide what they'd like to do with
/// this. For example, Mir windows may set their position according
/// to the provided rectangle on each frame, while outputs may change
/// their current workspace to show a different workspace.
struct AnimationFrameResult
{
    /// Whether this result marks the end of animation.
    ///
    /// Once set, the animation that produced this object will
    /// be removed.
    bool is_complete = false;

    /// The current rectangle set by the animation, if any.
    std::optional<mir::geometry::Rectangle> rectangle;

    /// The current transform set by the animation, if any.
    std::optional<glm::mat4> transform;

    /// The current opacity set by the animation, if any.
    std::optional<float> opacity;

    AnimationFrameResult merge(AnimationFrameResult const& other) const;
};

/// An animation managed by the #Animator.
///
/// When an animation is created, it is provided with an
/// #AnimationDefinition that defines how the animation should
/// behave.
class Animation
{
public:
    /// Construct a new animation.
    ///
    /// \param handle The handle of the animated object.
    /// \param definition The definition of the animation.
    /// \param data The data for the animation.
    Animation(
        AnimationHandle handle,
        AnimationDefinition const& definition,
        AnimationData&& data,
        std::function<void(AnimationFrameResult const&)>&& on_tick,
        std::shared_ptr<PluginManager> const& plugin_manager);
    virtual ~Animation() = default;
    [[nodiscard]] AnimationHandle handle() const;
    virtual void mark_for_removal();
    [[nodiscard]] virtual bool is_being_removed() const;
    bool tick(float dt);

private:
    AnimationFrameResult tick_built_in(BuiltInAnimationDefinition const& builtin_def, float t);
    AnimationFrameResult finish() const;

    float runtime_seconds = 0.f;
    AnimationHandle handle_;
    AnimationDefinition definition_;
    AnimationData data_;
    std::function<void(AnimationFrameResult const&)> on_tick;
    bool is_being_removed_ = false;
    std::shared_ptr<PluginManager> plugin_manager;
};
}

#endif // MIRACLE_WM_ANIMATION_H